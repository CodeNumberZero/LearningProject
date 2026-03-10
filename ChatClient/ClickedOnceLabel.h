#pragma once
#include "global.h"

// 支持点击一次的label功能
class ClickedOnceLabel : public QLabel
{
    Q_OBJECT
public:
    ClickedOnceLabel(QWidget* parent = nullptr);
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
signals:
    void clicked(QString);
};

