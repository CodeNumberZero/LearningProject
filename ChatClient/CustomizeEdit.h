#pragma once
#include "global.h"

class CustomizeEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget* parent = nullptr);
    ~CustomizeEdit();
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent* event) override;                   // override的作用：让编译器帮助检查是否正确地重写了基类函数;重写时可以省略virtual(补充:final的作用是禁止进一步重写)

private:
    void limitTextLength(QString text);

    int _max_len;
signals:
    void sig_foucus_out();
    void sig_mouse_clicked();
};

