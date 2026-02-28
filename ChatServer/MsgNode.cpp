#include "MsgNode.h"

MsgNode::MsgNode(short max_len) : _total_len(max_len), _cur_len(0) {
	_data = new char[_total_len + 1]();     // 括号的作用是将分配的字符数组的所有字节初始化为\0(空字符)
	_data[_total_len] = '\0';
}

MsgNode::~MsgNode() {
	delete[] _data;
	std::cout << "MsgNode destruct!" << std::endl;
}

void MsgNode::Clear() {
	memset(_data, 0, _total_len);
	_cur_len = 0;
}

ReceNode::ReceNode(short max_len, short msg_id) : MsgNode(max_len), _msg_id(msg_id) {}

short ReceNode::GetMsgId() const{
	return _msg_id;
}

SendNode::SendNode(const char* msg, short max_len, short msg_id) : MsgNode(max_len + HEAD_TOTAL_LENGTH), _msg_id(msg_id) {
	// 先发送id
	short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id); // 转为网络字节序
	memcpy(_data, &msg_id_net, HEAD_ID_LENGTH);

	// 再发送消息长度
	short max_len_net = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	memcpy(_data + HEAD_ID_LENGTH, &max_len_net, HEAD_DATA_LENGTH);

	// 最后发送消息内容
	memcpy(_data + HEAD_TOTAL_LENGTH, msg, max_len);
	_data[_total_len] = '\0';
}

short SendNode::GetMsgId() const {
	return _msg_id;
}