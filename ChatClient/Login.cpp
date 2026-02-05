#include "Login.h"
#include <qmediaplayer.h>
#include <qvideowidget.h>
//#include <qurl.h>

Login::Login(QWidget *parent)
	: QDialog(parent)
{
    ui.setupUi(this);  // 初始化ui
    connect(ui.register_Button, &QPushButton::clicked, this, &Login::sigSwitchRegister);

    ui.forget_label->SetState("normal", "hover", "", "selected", "selected_hover", "");
    ui.forget_label->setCursor(Qt::PointingHandCursor);
    connect(ui.forget_label, &ClickedLabel::clicked, this, &Login::slot_forget_pwd);

    //QMediaPlayer* player = new QMediaPlayer(this);
    //QVideoWidget* videoWidget = new QVideoWidget();
    //player->setVideoOutput(videoWidget);
    //player->setSource(QUrl("qrc:/video/sword.mp4")); // 设置MP4文件路径

    //connect(player, &QMediaPlayer::positionChanged, [=](qint64 position) {
    //    qint64 duration = player->duration();
    //    if (duration > 0 && position >= duration - 100) { // 接近视频末尾（留100ms余量）
    //        player->setPosition(0); // 重置到开头
    //        player->play(); // 重新播放
    //    }
    //    });

    ////ui.verticalLayout->addWidget(videoWidget);   // 将视频控件添加到布局
    //ui.verticalLayout->insertWidget(0, videoWidget);  //将视频控件插入到布局的最上方（位置0）

    //player->play();  // 启动播放（可选：添加按钮控制播放/暂停）
}

Login::~Login()
{
    qDebug() << "Login destruct!";
}

void Login::slot_forget_pwd() {
    qDebug() << "slot forget pwd!";
    emit sigSwitchReset();
}