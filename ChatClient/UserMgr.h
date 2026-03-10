#pragma once
#include "global.h"
#include "Singleton.h"

class UserMgr : public QObject, public Singleton<UserMgr>, public std::enable_shared_from_this<UserMgr>
{
	Q_OBJECT
private:
	UserMgr();
	QString _name;
	QString _token;
	int _uid;
	//std::vector<std::shared_ptr<ApplyInfo>> _apply_list;

public:
	friend class Singleton<UserMgr>;
	~UserMgr();
	void SetName(QString name);
	void SetUid(int uid);
	void SetToken(QString token);
	int GetUid();
	QString GetName();
	//void AppendApplyList(QJsonArray array);
	//std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
};

