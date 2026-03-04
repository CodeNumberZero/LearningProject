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

protected:
    //virtual void paintEvent(QPaintEvent* event) override;               // 因为ListItemBase继承了QWidget,而QWidget是很基本的组件,所以我们想实现更复杂的样式更新，就需要重写paintEvent(主要作用是用于正确绘制自定义 QWidget 的背景样式,确保控件的外观与当前样式一致)

private:
    ListItemType _itemType;

public slots:

signals:
};

