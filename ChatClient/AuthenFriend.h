#pragma once

#include <QDialog>
#include "ui_AuthenFriend.h"
#include "ClickedLabel.h"
#include "UserData.h"
#include "FriendLabel.h"

// 好友认证界面与好友申请界面(ApplyFriend的ui文件和源文件)非常类似,主要改动SlotApplySure和SlotApplyCancel这两个函数即可
class AuthenFriend : public QDialog
{
	Q_OBJECT

public:
	AuthenFriend(QWidget *parent = nullptr);
	~AuthenFriend();

	void InitTipLbs();
	void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint& next_point, int text_width, int text_height);
	bool eventFilter(QObject* obj, QEvent* event);
	void SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info);

private:
	Ui::AuthenFriendClass ui;

    QMap<QString, ClickedLabel*> _add_labels;                                  // 使用QMap存储已经创建好的标签
    std::vector<QString> _add_label_keys;
    QPoint _label_point;
    QMap<QString, FriendLabel*> _friend_labels;                                // 用来在输入框显示添加新好友的标签
    std::vector<QString> _friend_label_keys;
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;
    std::shared_ptr<ApplyInfo> _apply_info;

    void resetLabels();
    void addLabel(QString name);

public slots:
    void ShowMoreLabel();                                                      // 显示更多label标签
    void SlotLabelEnter();                                                     // 输入label按下回车触发将标签加入展示栏
    void SlotRemoveFriendLabel(QString);                                       // 点击关闭，移除展示栏好友便签
    void SlotChangeFriendLabelByTip(QString, ClickLbState);                    // 通过点击tip实现增加和减少好友便签
    void SlotLabelTextChange(const QString& text);                             // 输入框文本变化显示不同提示
    void SlotLabelEditFinished();                                              // 输入框输入完成
    void SlotAddFirendLabelByClickTip(QString text);                           // 输入标签显示提示框，点击提示框内容后添加好友便签
    void SlotAuthenSure();                                                      // 处理确认回调
    void SlotAuthenCancel();                                                    // 处理取消回调
};

