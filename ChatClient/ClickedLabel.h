#pragma once
#include "global.h"

class ClickedLabel : public QLabel
{
	Q_OBJECT
public:
    ClickedLabel(QWidget* parent);
    virtual void mousePressEvent(QMouseEvent* event) override;                                // 处理鼠标点击事件
    virtual void enterEvent(QEnterEvent* event) override;                                     // 处理鼠标悬停进入事件
    virtual void leaveEvent(QEvent* event) override;                                          // 处理鼠标悬停离开事件
    void SetState(QString normal = "", QString hover = "", QString press = "",
        QString select = "", QString select_hover = "", QString select_press = "");
    ClickLbState GetCurState();
protected:
private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;
    QString _selected;
    QString _selected_hover;
    QString _selected_press;
    ClickLbState _curstate;
signals:
    void clicked(void);
};

