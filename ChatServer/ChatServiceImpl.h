#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <mutex>
#include "data.h"
#include "const.h"
#include "CServer.h"

//using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

using message::KickUserReq;
using message::KickUserRsp;

// 聊天服务的实现类，继承自ChatService::Service，重写了NotifyAddFriend、NotifyAuthFriend和NotifyTextChatMsg这几个RPC方法
class ChatServiceImpl final : public ChatService::Service
{
public:
	ChatServiceImpl();
	Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply) override;
	Status NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* reply) override;
	Status NotifyTextChatMsg(::grpc::ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* reply) override;
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	Status NotifyKickUser(ServerContext* context, const KickUserReq* request, KickUserRsp* reply) override;
	void RegisterServer(std::shared_ptr<Server> p_server);							// 这个函数在main函数中启动grpc服务前注册即可
private:
	std::shared_ptr<Server> _p_server;
};

