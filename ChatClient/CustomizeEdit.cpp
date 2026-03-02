#include "CustomizeEdit.h"

CustomizeEdit::CustomizeEdit(QWidget* parent) : QLineEdit(parent), _max_len(0)
{
    connect(this, &QLineEdit::textChanged, this, &CustomizeEdit::limitTextLength); // 当行编辑框里的文本内容发生任何变化时，自动发射textChanged信号
}

CustomizeEdit::~CustomizeEdit()
{
}

void CustomizeEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent* event)
{
    // 执行失去焦点时的处理逻辑
    // qDebug() << "CustomizeEdit focusout";
    // 调用基类的focusOutEvent()方法，保证基类的行为得到执行
    QLineEdit::focusOutEvent(event);
    //发送失去焦点的信号
    emit sig_foucus_out();
}

void CustomizeEdit::limitTextLength(QString text) {
    if (_max_len <= 0) {
        return;
    }

    QByteArray byteArray = text.toUtf8();                  // 不同情况下一个汉字或字母所占用的字节数可能不同，因此统一转为字节数组进行计数

    if (byteArray.size() > _max_len) {                     // 如果输入超过最大长度限制则进行截取
        byteArray = byteArray.left(_max_len);
        this->setText(QString::fromUtf8(byteArray));
    }
}