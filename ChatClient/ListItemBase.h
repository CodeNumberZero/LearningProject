#pragma once
#include "global.h"

// 用于控制item的基类，任何一个item都可以继承这个基类
class ListItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget* parent = nullptr);
    void SetItemType(ListItemType itemType);
    ListItemType GetItemType();

private:
    ListItemType _itemType;

public slots:

signals:
};

