#pragma once
#include "BubbleFrame.h"

// 用于显示图片消息的气泡类
class PictureBubble : public BubbleFrame
{
    Q_OBJECT
public:
    PictureBubble(const QPixmap& picture, ChatRole role, QWidget* parent = nullptr);
};
