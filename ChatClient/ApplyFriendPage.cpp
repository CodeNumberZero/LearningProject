#include "ApplyFriendPage.h"
#include "ApplyFriendList.h"
#include "TcpMgr.h"
#include "UserMgr.h"
#include "AuthenFriend.h"

ApplyFriendPage::ApplyFriendPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.apply_friend_list, &ApplyFriendList::sig_show_search, this, &ApplyFriendPage::sig_show_search); // 通过信号传递另一个信号
	loadApplyList();
	//接受tcp传递的authrsp信号处理
	connect(TcpMgr::GetInstance().get(), &TcpMgr::sigAuthRsp, this, &ApplyFriendPage::slot_auth_rsp);
}

ApplyFriendPage::~ApplyFriendPage()
{}

// 模拟添加好友的逻辑
void ApplyFriendPage::AddNewApply(std::shared_ptr<AddFriendApply> apply)
{
	// 先模拟头像随机，以后头像资源增加资源服务器后再显示
	int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
	int head_i = randomValue % heads.size();

	auto* apply_item = new ApplyFriendItem();
	auto apply_info = std::make_shared<ApplyInfo>(apply->_from_uid, apply->_name, apply->_desc, heads[head_i], apply->_name, 0, 0);
	apply_item->SetInfo(apply_info);

	QListWidgetItem* item = new QListWidgetItem;
	// qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
	item->setSizeHint(apply_item->sizeHint());
	item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
	ui.apply_friend_list->insertItem(0, item);
	ui.apply_friend_list->setItemWidget(item, apply_item);
	apply_item->ShowAddBtn(true);

    /*
        截取自弹幕：在AddNewApply里把新uid进入map就行了
        待解决,对应的具体问题为:
        当双方都在线时，a向b发送申请，b同意后，a的聊天列表和联系人列表会更新，但是b的不会，且b的申请列表也不会更新为"已添加"
    */
    auto uid = apply_item->GetUid(); // 这两句不能少,否则会因为这里的添加item项函数没有把item加到map里导致好友认证方完成认证后不会将界面刷新为"已添加"(load初始化的时候加了这两句的逻辑,和88行对比着看)
    _unauth_items[uid] = apply_item;

    // 每当有item放到申请页面时都要把itenm对应的信号和槽连接好,便于每个item在处理自己的点击事件时能正常触发回调
	// 收到审核好友信号(A向B发送了好友申请,B这边显示了申请信息并点击了"添加"按钮后发送sig_friend_auth信号,根据该信号触发对应的回调函数)
	connect(apply_item, &ApplyFriendItem::sig_friend_auth, [this](std::shared_ptr<ApplyInfo> apply_info) {
		auto* authFriend = new AuthenFriend(this);
		authFriend->setModal(true);
		authFriend->SetApplyInfo(apply_info);
		authFriend->show();
	});
}

// 重写该函数，使其可以加载样式表
void ApplyFriendPage::paintEvent(QPaintEvent* event)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ApplyFriendPage::loadApplyList()
{
    //添加好友申请
    auto apply_list = UserMgr::GetInstance()->GetApplyList();
    for (auto& apply : apply_list) {
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int head_i = randomValue % heads.size();

        auto* apply_item = new ApplyFriendItem();
        apply->SetIcon(heads[head_i]);
        apply_item->SetInfo(apply);

        QListWidgetItem* item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(apply_item->sizeHint());
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        ui.apply_friend_list->insertItem(0, item);
        ui.apply_friend_list->setItemWidget(item, apply_item);
        if (apply->_status) {
            apply_item->ShowAddBtn(false);
        }
        else {
            apply_item->ShowAddBtn(true);
            auto uid = apply_item->GetUid(); // 有了这两句就可以：当b不在线，a向b发送申请后，b上线后同意申请会更新ui，且申请列表会刷新为"已添加"(和43行对比着看)
            _unauth_items[uid] = apply_item;
        }

        // 每当有item放到申请页面时都要把itenm对应的信号和槽连接好,便于每个item在处理自己的点击事件时能正常触发回调
        // 收到审核好友信号(A向B发送了好友申请,B这边显示了申请信息并点击了"添加"按钮后发送sig_friend_auth信号,根据该信号触发对应的回调函数)
        connect(apply_item, &ApplyFriendItem::sig_friend_auth, [this](std::shared_ptr<ApplyInfo> apply_info) {
            auto* authFriend = new AuthenFriend(this);
            authFriend->setModal(true);
            authFriend->SetApplyInfo(apply_info);
            authFriend->show();
        });
    }

    // 模拟假数据，创建QListWidgetItem，并设置自定义的widget
    for (int i = 0; i < 13; i++) {
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue % strs.size();
        int head_i = randomValue % heads.size();
        int name_i = randomValue % names.size();

        auto* apply_item = new ApplyFriendItem();
        auto apply = std::make_shared<ApplyInfo>(0, names[name_i], strs[str_i],
            heads[head_i], names[name_i], 0, 1);
        apply_item->SetInfo(apply);
        QListWidgetItem* item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(apply_item->sizeHint());
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        ui.apply_friend_list->addItem(item);
        ui.apply_friend_list->setItemWidget(item, apply_item);

        // 每当有item放到申请页面时都要把itenm对应的信号和槽连接好,便于每个item在处理自己的点击事件时能正常触发回调
        // 收到审核好友信号(A向B发送了好友申请,B这边显示了申请信息并点击了"添加"按钮后发送sig_friend_auth信号,根据该信号触发对应的回调函数)
        connect(apply_item, &ApplyFriendItem::sig_friend_auth, [this](std::shared_ptr<ApplyInfo> apply_info) {
            auto* authFriend = new AuthenFriend(this);
            authFriend->setModal(true);
            authFriend->SetApplyInfo(apply_info);
            authFriend->show();
        });
    }
}

void ApplyFriendPage::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto uid = auth_rsp->_uid;
    auto find_iter = _unauth_items.find(uid);
    if (find_iter == _unauth_items.end()) {
        return;
    }
    find_iter->second->ShowAddBtn(false);
}