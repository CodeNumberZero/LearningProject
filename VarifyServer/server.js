
const grpc = require('@grpc/grpc-js')
const message_proto = require('./proto')
const const_module = require('./const')
// const { v4: uuidv4 } = require('uuid');
const emailModule = require('./email');
const redis_module = require('./redis')

/**
 * GetVarifyCode grpc响应获取验证码的服务
 * @param {*} call 为grpc请求 
 * @param {*} callback 为grpc回调
 * @returns 
 */
async function GetVarifyCode(call, callback) {
    const clientPeer = call.getPeer();                                                             // 1. 获取客户端地址（IP+端口）
    const connectTime = new Date().toLocaleString();                                               // 2. 获取当前时间
    console.log(`\n[连接成功]${connectTime} → 客户端地址：${clientPeer}`);                           // 3. 第一时间打印连接成功的调试信息
    console.log(`[请求参数]客户端发送的原始数据：`, JSON.stringify(call.request, null, 2));           // 4. 获取客户端请求体

    console.log("Server receive email is ", call.request.email)
    try{
        const { v4: uuidv4 } = await import ('uuid'); 
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email);  // 获取验证码之前先查询redis，如果没查到就生成uid并且写入redis
        console.log("query_res is ", query_res)
        let uniqueId = query_res;
        if(query_res != null)
        {
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4);
            }
        }
        if(query_res ==null){
            uniqueId = uuidv4();
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4);
            } 
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId,180)
            if(!bres){
                callback(null, { email:  call.request.email,
                    error:const_module.Errors.RedisErr
                });
                return;
            }
        }

        console.log("uniqueId is ", uniqueId)
        let text_str =  '您的验证码为:'+ uniqueId +',请三分钟内完成注册'
        //发送邮件
        let mailOptions = {
            from: 'feng_pengxi@163.com',
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };
    
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Success
        }); 
        
 
    }catch(error){
        console.log("catch error is ", error)

        callback(null, { email:  call.request.email,
            error:const_module.Errors.Exception
        }); 
    }
     
}

function main() {
    var server = new grpc.Server()
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), (err, port) => {
        // console.log('grpc server started')   
        if (err) {
            console.error('[gRPC服务端]启动失败,无法监听端口:', err);
            return;
        }
        console.log(`[gRPC服务端]已启动,监听 0.0.0.0:${port} → 等待客户端连接...`);
    })
}

main()
