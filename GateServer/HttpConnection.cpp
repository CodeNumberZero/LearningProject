#include "HttpConnection.h"
#include "LogicSystem.h"

// 将单个unsigned char类型的数字(0~15)转换为对应的十六进制字符(0~9、A~F)
// 数字字符'0'的ASCII码值是48;大写字母'A'的ASCII码值是65;小写字母'a'的ASCII码值是97
unsigned char ToHex(unsigned char x)                       // 在这个函数中参数x是一个表示数值的unsigned char类型的整数(0~15),而不是一个字符,因此在运算时不会使用其对应的ASCII码进行运算,而是会进行隐式转换
{
    return  x > 9 ? x + 55 : x + 48;                       // 这一步涉及到:unsigned char类型的参数x先隐式转换(提升)为int类型,计算完成后再将int结果隐式转换回unsigned char类型返回
}

// 将单个十六进制字符(0~9、A~Z、a~z)转换为对应的十进制数字(0~15)
unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

// url编码
std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //判断是否仅有数字和字母构成,或者一些简单的下划线，如果是则直接拼接
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //为空字符则将空格字符直接替换为'+'并拼接到结果中
            strTemp += "+";
        else
        {
            //其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);     // 示例:字符'&'的ASCII码是38,对应二进制00100110,右移4位后得到00000010(十进制2),ToHex(2)返回'2',这样就提取了高4位并转为了十六进制
            strTemp += ToHex((unsigned char)str[i] & 0x0F);   // 示例:字符'&'的ASCII码是38(对应二进制00100110),与0x0F(对应二进制00001111)按位与后，得到00000110(十进制6),ToHex(6)返回'6',这样就提取了低4位并转为了十六进制.最终字符'&'被编码为%26
        }
    }
    return strTemp;
}

// url解码
std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //还原+为空
        if (str[i] == '+') strTemp += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);  // ++i让索引指向%后面的第1个字符(高位十六进制字符)并解码
            unsigned char low = FromHex((unsigned char)str[++i]);   // 再次++i让索引指向%后面的第2个字符(低位十六进制字符)并解码
            strTemp += high * 16 + low;                             // 十六进制转十进制的计算,高位数字乘以16(对应十六进制的进位规则,相当于左移4位),加上低位数字,得到原始字符的ASCII码值.示例:高位2×16+低位6=38,对应ASCII字符'&',完成%26到'&'的还原
        }
        else strTemp += str[i];
    }
    return strTemp;
}

// _socket是HttpConnection类的成员变量，如果不显式构造就会调用默认构造，但socket不存在默认构造和拷贝构造，所以需要在初始化列表中显式构造(使用移动构造) 
HttpConnection::HttpConnection(boost::asio::io_context& ioc) : _socket(ioc)
{
}

void HttpConnection::Start() {
	auto self = shared_from_this();
    boost::beast::http::async_read(_socket, _buffer, _request, [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
        try {
            if (ec) {                                      // boost::beast::error_code是一个class，它能直接用if(ec)判断的原因是重载了bool()用于隐式转换
                std::cout << "http read err is " << ec.what() << std::endl;
                return;
            }
            boost::ignore_unused(bytes_transferred);
            self->HandleReq();
            self->CheckDeadline();
        }
        //catch (std::exception& e) {
        //    std::cout << "HttpConnection::Start occurred exception. Exception is " << e.what() << std::endl;
        //}
        catch (boost::system::error_code & e) {
            std::cout << "HttpConnection::Start occurred exception. Exception is " << e.what() << ", value is " << e.value()
                << ", message is " << e.message() << std::endl;
        }
     });
}

boost::asio::ip::tcp::socket& HttpConnection::GetSocket()
{
    return _socket;
}

void HttpConnection::CheckDeadline()
{
    auto self = shared_from_this();                                                // 成员函数使用share_from_this的前提条件是创建这个类的时候一定会以智能指针的形式创建
    _deadline.async_wait([self](boost::system::error_code ec) {                    // 定时器的异步等待函数(超时后调用)，接受一个可调用对象
        if (!ec) {
            self->_socket.close(ec);
            // 这里不能直接使用this，而是必须将self传进来进行调用
            // (因为如果使用this,当http_connection类由于特殊原因提前释放后，this指针就无效了，函数就无法正常调用；
            // 而把self传进来会增加智能指针的引用计数，延长类的生命周期，保证在定时器回调执行期间，对象不会被析构,直到函数调用完毕才会释放，可以避免上述问题)    
        }
    });
}

void HttpConnection::WriteResponse()
{
    auto self = shared_from_this();
    _response.content_length(_response.body().size());                             // 设置HTTP响应头中的Content-Length字段，其值为响应体(_response.body())的字节大小
    boost::beast::http::async_write(_socket, _response, [self](boost::beast::error_code ec, std::size_t) {  // 将_response序列化为HTTP原始报文，异步写入到_socket(客户端连接)中，发送给客户端
        self->_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);   // 关闭服务器的发送端
        self->_deadline.cancel();                                                  // 触发该回调说明http请求已处理完，取消定时器
    });
}

void HttpConnection::HandleReq()
{
    _response.version(_request.version());                                         // 设置回应版本
    _response.keep_alive(false);                                                   // 设为false即为短链接
    if (_request.method() == boost::beast::http::verb::get) {                      // 根据HTTP请求方(GET/POST/其他)，构建不同的响应结果
        PreParseGetParam();                                                        // 解析HTTP GET请求
        bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
        if (!success) {
            _response.result(boost::beast::http::status::not_found);               // 设置响应状态码为404 not_found
            _response.set(boost::beast::http::field::content_type, "text/plain");  // 设置响应头Content-Type:text/plain，告诉客户端响应体是纯文本格式，便于客户端解析
            boost::beast::ostream(_request.body()) << "url not found\r\n";
            WriteResponse();
            return ;
        }
        _response.result(boost::beast::http::status::ok);                          // 设置响应状态码为200 OK，表示请求被成功处理并返回正常结果
        _response.set(boost::beast::http::field::server, "GateServer");            // 设置响应头Server:GateServer，告诉客户端当前处理请求的服务器软件是GateServer
        WriteResponse();
        return;
    }

    if (_request.method() == boost::beast::http::verb::post) {
        bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
        if (!success) {
            _response.result(boost::beast::http::status::not_found);
            _response.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.result(boost::beast::http::status::ok);
        _response.set(boost::beast::http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::PreParseGetParam()
{
    // 提取 URI  
    auto uri = _request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）  
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {                                          // std::string::npos表示未找到
        _get_url = uri;                                                            // 如果URI中没有?(说明没有查询参数),则将整个URI赋值给_get_url(纯路径),直接返回
        return;
    }
    _get_url = uri.substr(0, query_pos);                                           // 截取0到?之前的部分(纯路径)
    std::string query_string = uri.substr(query_pos + 1);                          // 截取?之后的部分(查询字符串).示例:URI为/user?name=Li+Lei&age=20 -> _get_url="/user",query_string="name=Li+Lei&age=20"
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos)); 
            value = UrlDecode(pair.substr(eq_pos + 1));
            _get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）  
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            _get_params[key] = value;
        }
    }
}
