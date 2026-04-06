#include "Chat.h"
#include "ChatUserWid.h"
#include "LoadingDialog.h"
#include "TcpMgr.h"
#include "UserMgr.h"
#include "ContactUserItem.h"

/*
    定义一些全局的变量用来做测试；
*/
//const std::vector<QString>  strs = { "hello world !",
//                             "nice to meet u",
//                             "New year，new life",
//                            "You have to love yourself",
//                            "My love is written in the wind ever since the whole world is you" };
//
//const std::vector<QString> heads = {
//    ":/image/resource/head_1.jpg",
//    ":/image/resource/head_6.jpg",
//    ":/image/resource/head_7.jpg",
//    ":/image/resource/head_19.jpg",
//    ":/image/resource/head_6.jpg"
//};
//
//const std::vector<QString> names = {
//    "zero-one",
//    "saber",
//    "revice",
//    "geat",
//    "gavv",
//    "zzz",
//    "python",
//    "rust"
//};

Chat::Chat(QWidget *parent)
	: QDialog(parent), _mode(ChatUIMode::ChatMode), _state(ChatUIMode::ChatMode), _b_loading(false), _last_widget(nullptr), _cur_chat_uid(0)
{
    /*
    传递this指针可以设置父对象关系,创建的控件都以 Login 为父对象
    这样做的目的是：
       - 控件会显示在 Login 上
       - 内存管理：当 Login 销毁时，子控件自动销毁
       - 事件传递：事件可以正确传递给父对象
    */
	ui.setupUi(this);                                                              // 初始化ui,将 ui 文件中的界面设置到 this 对象上,之后可以访问界面上的控件(将设计师设计的界面(.ui 文件)实例化到当前对象上)
	ui.add_Button->SetState("normal", "hover", "press");                           // 显式调用设置一下ClickedButton的三种状态
    //ui.add_Button->setProperty("state", "normal");
    ui.search_lineEdit->SetMaxLength(50);                                          // 设置搜索框可输入的最大字节数

    ui.stackedWidget->setCurrentWidget(ui.chat_page);                              // 设置登录后的中心部件为chatpage

    /* ----------------------------------------- 搜索栏设置 --------------------------------------------*/
    // 设置搜索框的默认状态和图标
    QAction* searchAction = new QAction(ui.search_lineEdit);                       // QAction 是 Qt 框架中用于表示用户命令或动作的类，代表一个可执行的操作，比如"新建文件"、"保存"、"复制"等
    searchAction->setIcon(QIcon(":/image/resource/search.png"));
    ui.search_lineEdit->addAction(searchAction, QLineEdit::LeadingPosition);       // 将放大镜图标放置到搜索框的最左边
    ui.search_lineEdit->setPlaceholderText(QStringLiteral("搜索"));                 // 设置搜索框中的默认文本

    // 创建一个清除动作并设置图标
    QAction* clearAction = new QAction(ui.search_lineEdit);
    clearAction->setIcon(QIcon(":/image/resource/close_transparent.png"));
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui.search_lineEdit->addAction(clearAction, QLineEdit::TrailingPosition);
    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui.search_lineEdit, &QLineEdit::textChanged, [clearAction](const QString& text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/image/resource/close_search.png"));
        }
        else {
            clearAction->setIcon(QIcon(":/image/resource/close_transparent.png")); // 文本为空时，切换回透明图标
        }
    });

    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui.search_lineEdit->clear();                                              // 清空文本
        clearAction->setIcon(QIcon(":/image/resource/close_transparent.png"));    // 清除文本后，切换回透明图标
        ui.search_lineEdit->clearFocus();                                         // 清除焦点
        //清除按钮被按下则不显示搜索框
        ShowSearch(false);
    });
    
    ShowSearch(false);                                                            // 默认情况下不显示搜索框

    // 这种写法要输入文本才会加载新页面,还有种写法是只要鼠标点击搜索栏就会加载新页面(这里的slot_text_changed槽可能会覆盖之前同一对象同一信号的connect函数,解决方法是可以将两个connect函数合并起来解决)
    connect(ui.search_lineEdit, &QLineEdit::textChanged, this, &Chat::slot_text_changed); // 链接搜索框输入变化;textChanged 信号会传递一个 QString 参数，包含输入框当前的文本内容

    // 检测鼠标点击位置判断是否要清空搜索框
    /* 
        1、安装事件过滤器:将一个对象(filterObj)安装为另一个对象的事件监视器; 被安装的对象的所有事件都会被filterObj先看到; filterObj可以在事件到达目标对象之前拦截和处理事件
        2、如果将当前对象安装为某个对象的事件过滤器,当前对象就必须重写eventFilter,因为installEventFilter只是注册了过滤器,但真正的过滤逻辑必须在eventFilter函数中实现;
           如果没有重写eventFilter,过滤器存在但没有实际作用,所有事件都不会被过滤，直接传递给目标对象
    */
    this->installEventFilter(this);                                               // 将当前对象安装为自身的事件过滤器,让Chat对象监视自己的所有事件,当有事件发生时,会先调用Chat::eventFilter函数

    ui.search_list->SetSearchEdit(ui.search_lineEdit);                            // 为SearchList设置search edit

    /* ---------------------------------------------- 列表设置 ----------------------------------*/
    AddChatUserList();                                                            // 添加聊天列表的功能是在本文件;添加联系人列表的功能在contactUserList.cpp文件第16行
    connect(ui.chat_user_list, &ChatUserList::sig_loading_chat_user, this, &Chat::slot_loading_chat_user);             // 连接加载聊天列表的信号和槽函数
    connect(ui.contact_user_list, &ContactUserList::sig_loading_contact_user, this, &Chat::slot_loading_contact_user); // 连接加载联系人列表的信号和槽函数
    connect(ui.contact_user_list, &ContactUserList::sig_switch_friend_info_page, this, &Chat::slot_friend_info_page);  // 连接点击联系人item发出的信号和用户信息展示槽函数
    connect(ui.contact_user_list, &ContactUserList::sig_switch_apply_friend_page, this, &Chat::slot_switch_apply_friend_page);     // 连接联系人页面点击好友申请条目的信号
    connect(ui.chat_user_list, &QListWidget::itemClicked, this, &Chat::slot_item_clicked);                             // 连接聊天列表点击信号

    SetSelectChatItem();     // 设置选中条目(该函数根据传入的id选中对应的item,不传参则默认选中第一条item)
    SetSelectChatPage();     // 更新聊天界面信息(该函数根据用户ID选中聊天item,并在右侧聊天区域显示对应的用户信息,不传参则默认选中第一条item并在右侧区域展示其对应信息)
    
    /* ---------------------------------------------- 侧边栏设置 --------------------------------*/
    QPixmap pixmap(":/image/resource/head_6.jpg");
    ui.side_head_label->setPixmap(pixmap);                                        // 将图片设置到QLabel上
    QPixmap scaledPixmap = pixmap.scaled(ui.side_head_label->size(), Qt::KeepAspectRatio); // 将图片缩放到label的大小
    ui.side_head_label->setPixmap(scaledPixmap);                                  // 将缩放后的图片设置到QLabel上
    ui.side_head_label->setScaledContents(true);                                  // 设置QLabel自动缩放图片内容以适应大小

    ui.side_chat_label->setProperty("state", "normal");
    ui.side_chat_label->SetState("normal", "hover", "pressed", "selected_normal", "selected_hover", "selected_pressed");
    ui.side_contact_label->SetState("normal", "hover", "pressed", "selected_normal", "selected_hover", "selected_pressed");

    AddLBGroup(ui.side_chat_label);
    AddLBGroup(ui.side_contact_label);

    connect(ui.side_chat_label, &StateWidget::clicked, this, &Chat::slot_side_chat);
    connect(ui.side_contact_label, &StateWidget::clicked, this, &Chat::slot_side_contact);

    ui.side_chat_label->SetSelected(true);                                        // 设置聊天label选中状态

    /* ---------------------------------------------- 好友申请相关 --------------------------------*/
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigFriendApply, this, &Chat::slot_friend_apply);      // 连接申请添加好友信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigAddFriendAuth, this, &Chat::slot_add_friend_auth); // 连接认证添加好友信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigAuthRsp, this, &Chat::slot_auth_rsp);              // 连接自己认证回复信号

    /* ---------------------------------------------- 跳转聊天界面相关 --------------------------------*/
	connect(ui.search_list, &SearchList::sigJumpChatItem, this, &Chat::slot_jump_chat_item);            // 搜索好友时如果是已经添加的好友点击后跳转到聊天界面,连接搜索列表发送的点击事件
    connect(ui.friend_info_page, &FriendInfoPage::sig_jump_chat_item, this, &Chat::slot_jump_chat_item_from_friendinfopage); // 连接好友信息界面发送的点击事件

    /* ----------------------------------------------文本消息发送相关 --------------------------------*/
    connect(ui.chat_page, &ChatPage::sig_append_send_chat_msg, this, &Chat::slot_append_send_chat_msg);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigTextChatMsg, this, &Chat::slot_text_chat_msg);     // 连接对端消息通知
}

Chat::~Chat()
{}

void Chat::AddChatUserList()
{
    // 先按照好友列表加载聊天记录，等以后客户端实现聊天记录数据库之后再按照最后信息排序
    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();
    if (friend_list.empty() == false) {
        for (auto& friend_ele : friend_list) {
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            if (find_iter != _chat_items_added.end()) {
                continue;
            }
            auto* chat_user_wid = new ChatUserWid();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_wid->SetInfo(user_info);
            QListWidgetItem* item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
			ui.chat_user_list->addItem(item);                                        // insertItem是在指定位置插入,addItem是添加到末尾
            ui.chat_user_list->setItemWidget(item, chat_user_wid);                   // 将一个自定义控件(chat_user_wid)关联到指定的列表项(item)上，替换默认的显示内容
            _chat_items_added.insert(friend_ele->_uid, item);                        // 如果已存在相同的key,则新value会覆盖旧value
        }
        // 更新已加载条目
        UserMgr::GetInstance()->UpdateChatLoadedCount();
    }


    // 模拟测试条目
    // 创建QListWidgetItem，并设置自定义的widget
    for (int i = 0; i < 30; i++) {
        int randomValue = QRandomGenerator::global()->bounded(100);                    // 生成0到99之间的随机整数
        int str_i = randomValue % strs.size();
        int head_i = randomValue % heads.size();
        int name_i = randomValue % names.size();

        auto* chat_user_wid = new ChatUserWid();
        auto user_info = std::make_shared<UserInfo>(0, names[name_i], names[name_i], heads[head_i], 0, strs[str_i]);
        chat_user_wid->SetInfo(user_info);
        QListWidgetItem* item = new QListWidgetItem;                                   // 创建列表项
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());                                  // 设置项的大小，使其与自定义控件的大小匹配
        ui.chat_user_list->addItem(item);                                              // 将项添加到列表
        ui.chat_user_list->setItemWidget(item, chat_user_wid);                         // 将自定义控件设置为该项的显示内容
    }
}

void Chat::ShowSearch(bool b_search)
{
    if (b_search) {
        ui.chat_user_list->hide();
        ui.contact_user_list->hide();
        ui.search_list->show();
        _mode = ChatUIMode::SearchMode;
    }
    else if (_state == ChatUIMode::ChatMode) {
        ui.chat_user_list->show();
        ui.contact_user_list->hide();
        ui.search_list->hide();
        _mode = ChatUIMode::ChatMode;
    }
    else if (_state == ChatUIMode::ContactMode) {
        ui.chat_user_list->hide();
        ui.search_list->hide();
        ui.contact_user_list->show();
        _mode = ChatUIMode::ContactMode;
    }
}

void Chat::AddLBGroup(StateWidget* lb)
{
    _lb_list.push_back(lb);
}

// 清除其他标签选中状态，只将被点击的标签设置为选中的效果
void Chat::ClearLabelState(StateWidget* lb)
{
    for (auto& ele : _lb_list) {
        if (ele == lb) {
            continue;
        }

        ele->ClearState();
    }
}

// 根据用户ID选中聊天列表中对应的聊天项
void Chat::SetSelectChatItem(int uid)
{
    if (ui.chat_user_list->count() <= 0) {                // 列表为空，直接返回;count获取列表项数量
        return;
    }

    if (uid == 0) {
        ui.chat_user_list->setCurrentRow(0);                      // 选中第一行
        QListWidgetItem* firstItem = ui.chat_user_list->item(0);  
        if (!firstItem) {
            return;
        }

        /*
            转为widget
            使用 itemWidget() 转为widget是因为：
            1、QListWidgetItem 本身只存储位置和状态信息，不存储显示的自定义控件
            2、QListWidgetItem是轻量级，管理列表行为，只是列表项的"占位符"，不负责显示，需要通过 itemWidget() 获取实际显示的控件
            3、ChatUserWid是重量级，负责显示
        */
        QWidget* widget = ui.chat_user_list->itemWidget(firstItem); // 获取第一项关联的自定义控件
        if (!widget) {
            return;
        }

        auto con_item = qobject_cast<ChatUserWid*>(widget);
        if (!con_item) {
            return;
        }
        _cur_chat_uid = con_item->GetUserInfo()->_uid;               // 从控件中提取用户信息,并保存当前选中的用户ID
        return;
    }

    // 在映射表中查找用户
    auto find_iter = _chat_items_added.find(uid);
    if (find_iter == _chat_items_added.end()) {
        qDebug() << "uid " << uid << " not found, set curent row 0";
        ui.chat_user_list->setCurrentRow(0);                        // 未找到，默认选中第一项
        return;
    }

    ui.chat_user_list->setCurrentItem(find_iter.value());           // 选中找到的项
    _cur_chat_uid = uid;                                            // 保存当前选中的用户ID
}

// 根据用户ID选中聊天页面，并在右侧聊天区域显示对应的用户信息
void Chat::SetSelectChatPage(int uid)
{
    if (ui.chat_user_list->count() <= 0) {                           // count获取列表项数量
        return;
    }

    if (uid == 0) {
        auto item = ui.chat_user_list->item(0);
        /*
            转为widget
            使用 itemWidget() 转为widget是因为：
            1、QListWidgetItem 本身只存储位置和状态信息，不存储显示的自定义控件
            2、QListWidgetItem是轻量级，管理列表行为，只是列表项的"占位符"，不负责显示，需要通过 itemWidget() 获取实际显示的控件
            3、ChatUserWid是重量级，负责显示
        */
        QWidget* widget = ui.chat_user_list->itemWidget(item);
        if (!widget) {
            return;
        }

        auto con_item = qobject_cast<ChatUserWid*>(widget);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui.chat_page->SetUserInfo(user_info);
        return;
    }

    auto find_iter = _chat_items_added.find(uid);
    if (find_iter == _chat_items_added.end()) {
        return;
    }

    //转为widget
    QWidget* widget = ui.chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    // 判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        auto con_item = qobject_cast<ChatUserWid*>(customItem);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui.chat_page->SetUserInfo(user_info);

        return;
    }
}

void Chat::loadMoreChatUser()
{
    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();
    if (friend_list.empty() == false) {
        for (auto& friend_ele : friend_list) {
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            if (find_iter != _chat_items_added.end()) {
                continue;
            }
            auto* chat_user_wid = new ChatUserWid();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_wid->SetInfo(user_info);
            QListWidgetItem* item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
            ui.chat_user_list->addItem(item);                                        // insertItem是在指定位置插入,addItem是添加到末尾
            ui.chat_user_list->setItemWidget(item, chat_user_wid);                   // 将一个自定义控件(chat_user_wid)关联到指定的列表项(item)上，替换默认的显示内容
            _chat_items_added.insert(friend_ele->_uid, item);                        // 如果已存在相同的key,则新value会覆盖旧value
        }
        // 更新已加载条目
        UserMgr::GetInstance()->UpdateChatLoadedCount();
    }
}

void Chat::loadMoreContactUser()
{
    auto friend_list = UserMgr::GetInstance()->GetContactListPerPage();
    if (friend_list.empty() == false) {
        for (auto& friend_ele : friend_list) {
            auto* chat_user_wid = new ContactUserItem();
            chat_user_wid->SetInfo(friend_ele->_uid, friend_ele->_name,
                friend_ele->_icon);
            QListWidgetItem* item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
            ui.contact_user_list->addItem(item);                                    // insertItem是在指定位置插入,addItem是添加到末尾
            ui.contact_user_list->setItemWidget(item, chat_user_wid);               // 将一个自定义控件(chat_user_wid)关联到指定的列表项(item)上，替换默认的显示内容
        }
        // 更新已加载条目
        UserMgr::GetInstance()->UpdateContactLoadedCount();
    }
}

bool Chat::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

void Chat::handleGlobalMousePress(QMouseEvent* event)
{
    // 实现点击位置的判断和处理逻辑
    // 先判断是否处于搜索模式，如果不处于搜索模式则直接返回
    if (_mode != ChatUIMode::SearchMode) {
        return;
    }
    // 将鼠标点击位置(全局坐标)转换为搜索列表自身坐标系中的位置
    QPoint posInSearchList = (ui.search_list->mapFromGlobal(event->globalPosition())).toPoint(); // qt6中globalPos()已经弃用,需要使用globalPosition()替代；如果需要QPoint(整数),还可以转换
    // 判断点击位置是否在聊天列表的范围内
    if (!ui.search_list->rect().contains(posInSearchList)) {
        // 如果不在聊天列表内，清空输入框
        ui.search_lineEdit->clear();
        ShowSearch(false);
    }
}

void Chat::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata)
{
    for (auto& msg : msgdata) {
		if (msg->_from_uid != _cur_chat_uid) {             // 先判断消息发送者是不是当前聊天页面对应的用户，如果不是则暂不处理(后续可以考虑在聊天列表上展示未读消息数量等信息)
            break;
        }

		ui.chat_page->AppendChatMsg(msg);                  // 如果消息发送者是当前聊天页面对应的用户则直接在聊天页面展示消息内容
    }
}

void Chat::slot_loading_chat_user() {
    if (_b_loading) {                                                                  // 防止在数据加载过程中重复触发加载函数,避免多次弹出加载对话框
        return;
    }

    _b_loading = true;
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);                                                     // 设置为模态对话框，阻塞用户对其他窗口的交互
    loadingDialog->show();                                                             // 显示对话框，但不会阻塞代码执行
    qDebug() << "add new data to Chat list.....";
    loadMoreChatUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();                                                      // 在当前事件循环结束后才真正删除(对话框显示后立即开始加载，加载完成后立即删除)

    _b_loading = false;
}

void Chat::slot_loading_contact_user()
{
    if (_b_loading) {
        return;
    }

    _b_loading = true;
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to Contact list.....";
    loadMoreContactUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}

void Chat::slot_side_chat()
{
    qDebug() << "receive side chat clicked";
    ClearLabelState(ui.side_chat_label);                                               // 清除其他标签选中状态，只将被点击的标签设置为选中的效果
    ui.stackedWidget->setCurrentWidget(ui.chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void Chat::slot_side_contact()
{
    qDebug() << "receive side contact clicked";
    ClearLabelState(ui.side_contact_label);                                            // 清除其他标签选中状态，只将被点击的标签设置为选中的效果
    //设置
    ui.stackedWidget->setCurrentWidget(ui.friend_apply_page);
    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void Chat::slot_text_changed(const QString& str)
{
    //qDebug()<< "receive slot text changed str is " << str;
    if (!str.isEmpty()) {                                                              // 搜索栏只要不为空则显示搜索列表
        ShowSearch(true);
        return;
    }
    ShowSearch(false);
}

void Chat::slot_friend_apply(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() << "Receive apply friend slot, applyuid is " << apply->_from_uid 
             << " name is " << apply->_name 
             << " desc is " << apply->_desc;

	bool b_already = UserMgr::GetInstance()->IsAlreadyApply(apply->_from_uid);           // 先检查是否已经存在相同的申请，如果已经存在则不再添加，避免重复添加同一条申请记录
    if (b_already) {
        return;
    }

	UserMgr::GetInstance()->AddApplyToList(std::make_shared<ApplyInfo>(apply));          // 不存在相同的申请记录，则将新的申请记录添加到申请列表中
    ui.side_contact_label->ShowRedPoint(true);                                           // 自己的想法:可以把展示红点的功能封装为槽函数,然后这里发送一个信号触发槽函数(同时也可以配合TcpMgr.cpp文件130行发出的信号)
    ui.contact_user_list->ShowRedPoint(true);
    ui.friend_apply_page->AddNewApply(apply);
}

// 客户端对sigAddFriendAuth的响应,实现添加好友到聊天列表中
void Chat::slot_add_friend_auth(std::shared_ptr<AuthInfo> auth_info)
{
    qDebug() << "receive slot_add_friend_auth uid is " << auth_info->_uid
             << " name is " << auth_info->_name 
             << " nick is " << auth_info->_nick;

    // 判断如果已经是好友则跳过
    auto b_friend = UserMgr::GetInstance()->CheckFriendById(auth_info->_uid);
    if (b_friend) {
        return;
    }
    UserMgr::GetInstance()->AddFriend(auth_info);                                        // 对方不在自己的好友列表里则添加对方好友

    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui.chat_user_list->insertItem(0, item);                                              // 在列表的指定位置插入一个空的列表项,第一个参数为插入位置(索引),第二个参数为要插入的列表项对象; insertItem是在指定位置插入,addItem是添加到末尾
    ui.chat_user_list->setItemWidget(item, chat_user_wid);                               // 将一个自定义控件（chat_user_wid）关联到指定的列表项（item）上，替换默认的显示内容
    _chat_items_added.insert(auth_info->_uid, item);                                     // 如果已存在相同的key,则新value会覆盖旧value
}

// 客户端对sigAuthRsp的响应,实现添加好友到聊天列表中
void Chat::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp) {
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
             << " name is " << auth_rsp->_name 
             << " nick is " << auth_rsp->_nick;

    // 判断如果已经是好友(即已经在聊天列表中)则跳过
    auto b_friend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if (b_friend) {
        return;
    }
    UserMgr::GetInstance()->AddFriend(auth_rsp);

    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui.chat_user_list->insertItem(0, item);                                       // 在列表的指定位置插入一个空的列表项,第一个参数为插入位置(索引),第二个参数为要插入的列表项对象; insertItem是在指定位置插入,addItem是添加到末尾
    ui.chat_user_list->setItemWidget(item, chat_user_wid);                        // 将一个自定义控件（chat_user_wid）关联到指定的列表项（item）上，替换默认的显示内容
    _chat_items_added.insert(auth_rsp->_uid, item);                               // 如果已存在相同的key,则新value会覆盖旧value
}

void Chat::slot_jump_chat_item(std::shared_ptr<SearchInfo> si)
{
    qDebug() << "slot jump chat item ";
    // 首先先查找聊天列表是否已存在该用户对应的聊天条目
    auto find_iter = _chat_items_added.find(si->_uid);
    if (find_iter != _chat_items_added.end()) {
        qDebug() << "jump to chat item , user uid is " << si->_uid;
        ui.chat_user_list->scrollToItem(find_iter.value());                  // 将列表中的指定项自动滚动到可见区域
        ui.side_chat_label->SetSelected(true);
        SetSelectChatItem(si->_uid);
        // 更新聊天界面信息
        SetSelectChatPage(si->_uid);
        slot_side_chat();
        return;
    }

    // 如果没找到，则创建新的条目插入到listwidget
    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(si);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui.chat_user_list->insertItem(0, item);                                // 在列表的指定位置插入一个空的列表项,第一个参数为插入位置(索引),第二个参数为要插入的列表项对象; insertItem是在指定位置插入,addItem是添加到末尾
    ui.chat_user_list->setItemWidget(item, chat_user_wid);                 // 将一个自定义控件（chat_user_wid）关联到指定的列表项（item）上，替换默认的显示内容
    _chat_items_added.insert(si->_uid, item);                              // 如果已存在相同的key,则新value会覆盖旧value

    ui.side_chat_label->SetSelected(true);
    SetSelectChatItem(si->_uid);
    //更新聊天界面信息
    SetSelectChatPage(si->_uid);
    slot_side_chat();
}

// 切换到好友信息页面并显示指定用户的信息
void Chat::slot_friend_info_page(std::shared_ptr<UserInfo> user_info)
{
    qDebug() << "receive switch friend info page sig";
    _last_widget = ui.friend_info_page;                               // 将当前页面(即将切换到的页面)保存为"上一个页面"
    ui.stackedWidget->setCurrentWidget(ui.friend_info_page);          // 将QStackedWidget的当前显示页面切换到好友信息页friend_info_page
    ui.friend_info_page->SetInfo(user_info);                          // 传入用户信息，更新页面显示内容
}

// 切换到好友申请页面
void Chat::slot_switch_apply_friend_page()
{
    qDebug() << "receive switch apply friend page sig";
    _last_widget = ui.friend_apply_page;
    ui.stackedWidget->setCurrentWidget(ui.friend_apply_page);
}

void Chat::slot_jump_chat_item_from_friendinfopage(std::shared_ptr<UserInfo> user_info)
{
    qDebug() << "slot jump chat item ";
    auto find_iter = _chat_items_added.find(user_info->_uid);          // 先查找目前聊天列表有无该用户对应的聊天条目
    if (find_iter != _chat_items_added.end()) {
        qDebug() << "jump to chat item , uid is " << user_info->_uid;
        ui.chat_user_list->scrollToItem(find_iter.value());            // 将列表中的指定项自动滚动到可见区域
        ui.side_chat_label->SetSelected(true);
        SetSelectChatItem(user_info->_uid);
        // 更新聊天界面信息
        SetSelectChatPage(user_info->_uid);
        slot_side_chat();
        return;
    }

    // 如果聊天列表没找到该用户对应的item，则创建新的item插入listwidget
    auto* chat_user_wid = new ChatUserWid();
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui.chat_user_list->insertItem(0, item);                             // 在列表的指定位置插入一个空的列表项,第一个参数为插入位置(索引),第二个参数为要插入的列表项对象; insertItem是在指定位置插入,addItem是添加到末尾
    ui.chat_user_list->setItemWidget(item, chat_user_wid);              // 将一个自定义控件（chat_user_wid）关联到指定的列表项（item）上，替换默认的显示内容
    _chat_items_added.insert(user_info->_uid, item);                    // 如果已存在相同的key,则新value会覆盖旧value

    ui.side_chat_label->SetSelected(true);
    SetSelectChatItem(user_info->_uid);
    // 更新聊天界面信息
    SetSelectChatPage(user_info->_uid);
    slot_side_chat();
}

// 该槽函数与ContactUserList.cpp文件中149行的slot_item_clicked函数非常类似
void Chat::slot_item_clicked(QListWidgetItem* item) {
    QWidget* widget = ui.chat_user_list->itemWidget(item); // 获取自定义widget对象
    if (!widget) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == ListItemType::INVALID_ITEM || itemType == ListItemType::GROUP_TIP_ITEM) {
        qDebug() << "slot invalid item clicked ";
        return;
    }

    if (itemType == ListItemType::CHAT_USER_ITEM) {
        // 创建对话框，提示用户
        qDebug() << "contact user item clicked ";

        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        auto user_info = chat_wid->GetUserInfo();
        //跳转到聊天界面
        ui.chat_page->SetUserInfo(user_info);
        _cur_chat_uid = user_info->_uid;
        return;
    }
}

void Chat::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata)
{
    if (_cur_chat_uid == 0) {
        return;
    }

    auto find_iter = _chat_items_added.find(_cur_chat_uid);
    if (find_iter == _chat_items_added.end()) {
        return;
    }

    // 转为widget
    QWidget* widget = ui.chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    // 判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        auto con_item = qobject_cast<ChatUserWid*>(customItem);
        if (!con_item) {
            return;
        }

        // 设置信息
        auto user_info = con_item->GetUserInfo();
        user_info->_chat_msgs.push_back(msgdata);

        std::vector<std::shared_ptr<TextChatData>> msg_vec;
        msg_vec.push_back(msgdata);
        UserMgr::GetInstance()->AppendFriendChatMsg(_cur_chat_uid, msg_vec);               // 将消息存储到用户管理器对应好友的聊天记录中,便于后续切换聊天item时加载聊天记录(这是针对发送方发送消息后的处理,即发送给对方的消息我们自己要缓存一下)

        return;
    }
}

void Chat::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg)
{
    auto find_iter = _chat_items_added.find(msg->_from_uid);
    if (find_iter != _chat_items_added.end()) {
        qDebug() << "set chat item msg, uid is " << msg->_from_uid;
        QWidget* widget = ui.chat_user_list->itemWidget(find_iter.value());
        auto chat_wid = qobject_cast<ChatUserWid*>(widget);
        if (!chat_wid) {
            return;
        }
        chat_wid->updateLastMsg(msg->_chat_msgs);
        UpdateChatMsg(msg->_chat_msgs);                                                    // 更新当前聊天页面记录(会根据消息发送者与当前聊天界面对应的用户是否一致进行不同处理)
		UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);      // 将消息存储到用户管理器对应好友的聊天记录中,便于后续切换聊天item时加载聊天记录(这是针对接收方接收消息后的处理,即对方发给我们的消息我们缓存一下)
        return;
    }

    // 如果没找到，则创建新的插入listwidget
    auto* chat_user_wid = new ChatUserWid();
    // 查询好友信息
    auto fi_ptr = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
    chat_user_wid->SetInfo(fi_ptr);
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    chat_user_wid->updateLastMsg(msg->_chat_msgs);
    UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);         // 将消息存储到用户管理器对应好友的聊天记录中,便于后续切换聊天item时加载聊天记录(这是针对接收方接收消息后的处理,即对方发给我们的消息我们缓存一下)

    ui.chat_user_list->insertItem(0, item);
    ui.chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(msg->_from_uid, item);
}
