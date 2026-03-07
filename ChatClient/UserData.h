#pragma once
#include <QString>

class SearchInfo {
public:
	SearchInfo(int uid, QString name, QString nick, QString desc, int sex);
	int _uid;
	QString _name;                  // Ãû×Ö
	QString _nick;                  // êÇ³Æ
	QString _desc;
	int _sex;
};

