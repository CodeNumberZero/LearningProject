#pragma once
#include <string>
#include <iostream>
#include <boost/asio.hpp>

#include "const.h"

// MsgNode基类
class MsgNode
{
public:
	MsgNode(short max_len);
	~MsgNode();
	void Clear();

	short _cur_len;
	short _total_len;
	char* _data;
};

// 子类，用于接收消息
class ReceNode : public MsgNode
{
public:
	ReceNode(short max_len, short msg_id);
	short GetMsgId() const;
private:
	short _msg_id;
};

// 子类，用于发送消息
class SendNode : public MsgNode
{
public:
	SendNode(const char* msg, short max_len, short msg_id);
	short GetMsgId() const;
private:
	short _msg_id;
};
