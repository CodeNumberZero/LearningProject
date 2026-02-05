#include "global.h"

QString gate_url_prefix = "";

std::function<void(QWidget*)> repolish = [](QWidget* w) {
	w->style()->unpolish(w);
	w->style()->polish(w);
};

std::function<QString(QString)> xorString = [](QString input) {
	QString result = input;
	int length = input.length();
	ushort xor_code = length % 255;                                                 // 计算异或密钥：长度对255取模，保证密钥是0-254的无符号短整型（ushort）
	for (int i = 0; i < length; ++i) {
		// 先获取字符的UTF-16Unicode值(ushort类型),与密钥进行按位异或运算,然后QChar()将运算后的数值重新构造为QChar字符
		result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ xor_code));
	}
	return result;
};

QString md5Encrypt(const QString& input)
{
	QByteArray byteArray = input.toUtf8();                                          // 将输入字符串转换为字节数组
	QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5); // 使用 MD5 进行加密
	return QString(hash.toHex());                                                   // 返回十六进制格式的加密结果
}

