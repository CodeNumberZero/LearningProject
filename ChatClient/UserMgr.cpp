#include "UserMgr.h"

UserMgr::UserMgr()
{
}

UserMgr::~UserMgr()
{
}

void UserMgr::SetName(QString name)
{
	_name = name;
}

void UserMgr::SetUid(int uid)
{
	_uid = uid;
}

void UserMgr::SetToken(QString token)
{
	_token = token;
}

int UserMgr::GetUid()
{
	return _uid;
}

QString UserMgr::GetName()
{
	return _name;
}

//void UserMgr::AppendApplyList(QJsonArray array)
//{
//    // 遍历 QJsonArray 并输出每个元素
//    for (const QJsonValue& value : array) {
//        auto name = value["name"].toString();
//        auto desc = value["desc"].toString();
//        auto icon = value["icon"].toString();
//        auto nick = value["nick"].toString();
//        auto sex = value["sex"].toInt();
//        auto uid = value["uid"].toInt();
//        auto status = value["status"].toInt();
//        auto info = std::make_shared<ApplyInfo>(uid, name,
//            desc, icon, nick, sex, status);
//        _apply_list.push_back(info);
//    }
//}
