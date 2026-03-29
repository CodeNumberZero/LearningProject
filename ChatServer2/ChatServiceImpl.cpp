#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"

ChatServiceImpl::ChatServiceImpl()
{
}

/*关于gRPC同一方法的不同参数的相关说明:查看StatusServiceImpl.cpp文件第15行*/

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply) {
	// 查找用户是否在本服务器(是否离线)
	auto touid = request->touid();
	auto session = UserMgr::GetInstance()->GetSession(touid);

	Defer defer([request, reply]() {
		reply->set_error(ErrorCodes::Success);
		reply->set_applyuid(request->applyuid());
		reply->set_touid(request->touid());
	});

	// 用户不在内存中(离线)则直接返回
	if (session == nullptr) {
		return Status::OK;
	}

	// 在内存中(在线)则直接利用对方连接的session发送通知给对方
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["applyuid"] = request->applyuid();
	rtvalue["name"] = request->name();
	rtvalue["desc"] = request->desc();
	rtvalue["icon"] = request->icon();
	rtvalue["sex"] = request->sex();
	rtvalue["nick"] = request->nick();

	std::string return_str = rtvalue.toStyledString();

	session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);               // 要包含CSession.h文件，否则session会报错:不允许使用指向不完整类型"Session"的指针或引用
	return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response) {
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* response) {
    return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    return true;
}