#include "PictureBubble.h"

const int PIC_MAX_WIDTH = 160;                                                               // 图片最大宽度 160px
const int PIC_MAX_HEIGHT = 90;                                                               // 图片最大高度 90px

PictureBubble::PictureBubble(const QPixmap& picture, ChatRole role, QWidget* parent)
    :BubbleFrame(role, parent)
{
    QLabel* lb = new QLabel();
    lb->setScaledContents(true);                                                             // 图片自动缩放适应标签大小
    QPixmap pix = picture.scaled(QSize(PIC_MAX_WIDTH, PIC_MAX_HEIGHT), Qt::KeepAspectRatio); // 原始图片可能很大，需要缩放到合适大小;scaled()缩放图片,QSize()是目标尺寸，Qt::KeepAspectRatio保持宽高比,防止图片变形
    lb->setPixmap(pix);                                                                      
    this->setWidget(lb);                                                                     // 将图片标签设置为气泡的内容

    int left_margin = this->layout()->contentsMargins().left();                              // 获取气泡布局的边距值
    int right_margin = this->layout()->contentsMargins().right();
    int v_margin = this->layout()->contentsMargins().bottom();
    setFixedSize(pix.width() + left_margin + right_margin, pix.height() + v_margin * 2);     // 计算并设置气泡大小
}