#include "MysqlDao.h"
#include "ConfigMgr.h"
#include "MysqlMgr.h"

MysqlPool::MysqlPool(const std::string& host, unsigned short port, const std::string& user, const std::string& passwd, const std::string& schema, int poolSize)
    : _host(host), _port(port), _user(user), _passwd(passwd), _schema(schema), _poolSize(poolSize), _b_stop(false)
{
    try {
        /*
            mysqlx/xdevapi.h文件使用的是XDevAPI(mysqlx::Session),该接口强制使用X Protocol协议，要求MySQL监听33060端口
            所以如果用传统协议的3308端口就会抛出异常
        */
        std::cout << "port = " << _port << std::endl;
        for (int i = 0; i < _poolSize; ++i) {
            auto session = std::make_unique<mysqlx::Session>(_host, _port, _user, _passwd, _schema);
            auto current_time = std::chrono::system_clock::now().time_since_epoch();
            long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
            _pool.push(std::make_unique<SqlConnection>(std::move(session), timestamp)); 
            /*
				如果直接把session传入SqlConnection的构造函数，会尝试拷贝unique_ptr，导致编译错误，所以需要使用std::move将session转换为右值引用，表示资源的所有权将被转移。
            */
        }

        _check_thread = std::thread([this]() {
            while (!_b_stop) {
                checkConnectionAlive();
				std::this_thread::sleep_for(std::chrono::seconds(60));      // 每隔60秒检查一次连接是否存活
            }
         });
        _check_thread.detach();
    }
    catch (const mysqlx::Error& e) {
        std::cout << "mysql pool init failed! exception is " << e.what() << std::endl;
    }
}

void MysqlPool::checkConnectionAlive()
{
    std::lock_guard<std::mutex> lock(_mutex);
    int poolsize = _pool.size();
    // 获取当前时间戳
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	// 将时间戳转换为秒
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
    for (int i = 0; i < poolsize; ++i) {
        auto conn = std::move(_pool.front());
        _pool.pop();
        /*
            Defer是延迟执行机制的统称，核心作用是让指定代码块在「当前作用域结束时」自动执行(无论作用域是正常结束、return退出还是异常抛出),本质是利用C++对象的析构函数特性实现的优雅封装,是C++中管理资源、保证操作收尾的常用编程技巧。
            简单来说:声明Defer后,绑定的代码会被"挂起",直到所在的{}代码块执行完毕,自动触发执行,类似Go语言的defer关键字、Python的with语句收尾逻辑。
        */
        Defer defer([this, &conn]() {
            _pool.push(std::move(conn));
        });

        if (timestamp - conn->_last_oper_time < 5) {                                  // 5秒内使用过的连接，跳过检测
            continue;
        }

        try {
            // 执行保活语句：SELECT 1
            conn->_con->sql("SELECT 1").execute();
            conn->_last_oper_time = timestamp;
            //std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
        }
        catch (const mysqlx::Error& e) {
            std::cerr << "Connection keep alive failed: " << e.what() << std::endl;
            // 重新创建连接，替换失效的Session
            auto new_session = std::make_unique<mysqlx::Session>(_host, _port, _user, _passwd, _schema);
			conn->_con = std::move(new_session);
			conn->_last_oper_time = timestamp;
        }
    }
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this] {
        if (_b_stop) {
            return true;
        }
        return !_pool.empty();
     });

    if (_b_stop) {
        return nullptr;
    }
	std::unique_ptr<SqlConnection> connection = std::move(_pool.front());
    _pool.pop();
    return connection;

}

void MysqlPool::returnConnection(std::unique_ptr<SqlConnection> connection)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _pool.push(std::move(connection));
    _cond.notify_one();
}

void MysqlPool::Close() {
    _b_stop = true;
    _cond.notify_all();
}

MysqlPool::~MysqlPool()
{
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_pool.empty()) {
        _pool.pop();
    }
}


MysqlDao::MysqlDao()
{
    auto& cfg = ConfigMgr::GetInstance();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port_str = cfg["Mysql"]["Port"];
    unsigned short port = atoi(port_str.c_str());
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    _pool.reset(new MysqlPool(host, port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao()
{
    _pool->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = _pool->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }
        
        con->_con->sql("CALL reg_user(?,?,?,@result)").bind(name, email, pwd).execute();       // 调用存储过程执行插入语句：使用占位符?，防止SQL注入
		mysqlx::SqlResult res = con->_con->sql("SELECT @result AS result").execute();          // 二次查询会话变量@result，获取存储过程OUT返回值
        mysqlx::Row row = res.fetchOne();                                                      // fetchOne()获取单行结果
        if (row) {
			int result = static_cast<int>(row.get(0));                                         // row.get(0)获取第0列值(因为存储过程返回值只有一个)
			std::cout << "RegUser result: " << result << std::endl;
            _pool->returnConnection(std::move(con));
            return result;
        }

        //auto result = con->_con->sql("INSERT INTO user (name, email, pwd) VALUES (?, ?, ?)").bind(name, email, pwd).execute();  // 直接执行INSERT语句，依赖MySQL自增主键机制返回ID
        // 获取自增ID
        //uint64_t uid = result.getAutoIncrementValue();

        _pool->returnConnection(std::move(con));
        return -1;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "RegUser failed: " << e.what() << std::endl;
        _pool->returnConnection(std::move(con));
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    auto conn = _pool->getConnection();
    try {
        if (conn == nullptr) {
            _pool->returnConnection(std::move(conn));
            return false;
        }
 
        // 根据name查询user表的email字段
        // 另一种写法:mysqlx::SqlResult res = conn->_con->sql("SELECT email FROM user WHERE name = ?").bind(name).execute();
        mysqlx::SqlStatement stmt = conn->_con->sql("SELECT email FROM user WHERE name = ?");
        stmt.bind(name);                                                            // 绑定参数：按占位符顺序绑定
        mysqlx::SqlResult res = stmt.execute();                                     // 执行查询，获取结果集
        

        //// 直接查询指定邮箱(考虑到用户名可能重名，邮箱才具有唯一性)
        //mysqlx::SqlResult res = conn->_con->sql("SELECT email FROM user WHERE email = ?").bind(email).execute();
        
        mysqlx::Row row = res.fetchOne();
        /*
            1、对于获取的单行数据，获取其中第n列数据(例如第4列的email数据)的两种写法：
                (1)可以使用列索引,如 row.get(3)和row[3].get<std::string>(),看源码就会发现重载[]运算符时就是调用了get方法而已
                (2)可以使用列名,如 row["email"].get<std::string>()
              如果查询语句的列顺序可能变化，建议使用列名访问，这样代码更健壮
            2、fetchOne()会消耗结果集的行,即调用fetchOne()后结果集的游标会移动到下一行,也就是说,每次调用fetchOne()都会获取下一行;可以通过调用res的hasData()方法常查看是否还有数据
            3、遍历多行结果集的方法:
                (1)直接对结果集res使用范围for循环和迭代器
                (2)对结果集res调用fetchAll()方法后得到rows,再对rows使用范围for循环和迭代器
                (3)使用hasData()和fetchOne()逐行提取
        */
        if (!row) {
            // 未查询到该用户名，返回false
            std::cerr << "Check eamil: " << email << " not found!" << std::endl;
            _pool->returnConnection(std::move(conn));
            return false;
        }

        // 提取email字段值
        //std::cout << row.colCount() << std::endl;   // 1
        //std::cout << row.isNull() << std::endl;     // 0
        std::string db_email = row[0].get<std::string>();
        std::cout << "Check email: " << db_email << " found!" << std::endl;

        bool match = (email == db_email);
        _pool->returnConnection(std::move(conn));
        return match;
    }
    catch (const mysqlx::Error& e) {
        _pool->returnConnection(std::move(conn));
        std::cerr << "Check email's SQLException: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& new_pwd) {
    auto conn = _pool->getConnection();
    try {
        if (conn == nullptr) {
            _pool->returnConnection(std::move(conn));
            return false;
        }
        // 准备并执行 UPDATE 语句
        mysqlx::SqlStatement stmt = conn->_con->sql("UPDATE user SET pwd = ? WHERE name = ?");

        // 绑定参数（注意：X DevAPI 的 bind 是按顺序的，不是按位置索引）
        stmt.bind(new_pwd);  // 第一个 ? 对应 pwd = ?
        stmt.bind(name);    // 第二个 ? 对应 name = ?
        mysqlx::SqlResult res = stmt.execute();

        // 获取受影响的行数
        long long updateCount = res.getAffectedItemsCount();
        std::cout << "Updated rows: " << updateCount << std::endl;   // Updated rows: 1

        _pool->returnConnection(std::move(conn));
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "UpdatePwd's SQLException: " << e.what() << std::endl;
        _pool->returnConnection(std::move(conn));
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
    auto conn = _pool->getConnection();
    if (conn == nullptr) {
        return false;
    }

    // 离开作用域后自动执行该语句
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
    });

    try {
        // 准备SQL语句
		mysqlx::SqlResult res = conn->_con->sql("SELECT * FROM user WHERE email = ?").bind(email).execute();  // 直接执行查询语句，获取结果集
        // * 表示获取满足条件的指定行的所有列
        // 或者这样写mysqlx::SqlResult res = conn->_con->sql("SELECT uid, name, pwd FROM user WHERE email = ?").bind(email).execute(); 明确各个列的顺序和类型
        std::string origin_pwd = "";
        if (!res.hasData()) {
            // 未查询到数据，返回false
            std::cerr << "Check pwd: " << email << " not found!" << std::endl;
            return false;
        }
        mysqlx::Row row = res.fetchOne();
        //std::cout << "row.colCount() = " << row.colCount() << std::endl;  // row.colCount() = 5
        origin_pwd = row[4].get<std::string>();
        std::cout << "Origin Password: " << origin_pwd << std::endl;

        // 验证密码
        if (pwd != origin_pwd) {
            return false;
        }
        // 填充用户信息
        userInfo.name = row[2].get<std::string>();
        userInfo.email = email;
        userInfo.uid = row[1].get<int>();
        userInfo.pwd = origin_pwd;

        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "CheckPwd's SQLException: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::TestProcedure(const std::string& email, int& uid, std::string& name) {
    return true;
}