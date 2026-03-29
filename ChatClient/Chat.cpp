#include "Chat.h"
#include "ChatUserWid.h"
#include "LoadingDialog.h"
#include "TcpMgr.h"
#include "UserMgr.h"

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
	: QDialog(parent), _mode(ChatUIMode::ChatMode), _state(ChatUIMode::ChatMode), _b_loading(false)
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
    connect(ui.chat_user_list, &ChatUserList::sig_loading_chat_user, this, &Chat::slot_loading_chat_user);
    AddChatUserList();

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

    /* ---------------------------------------------- 好友申请 --------------------------------*/
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigFriendApply, this, &Chat::slot_friend_apply);   // 连接申请添加好友信号

}

Chat::~Chat()
{}

void Chat::AddChatUserList()
{
    // 创建QListWidgetItem，并设置自定义的widget
    for (int i = 0; i < 30; i++) {
        int randomValue = QRandomGenerator::global()->bounded(100);                    // 生成0到99之间的随机整数
        int str_i = randomValue % strs.size();
        int head_i = randomValue % heads.size();
        int name_i = randomValue % names.size();

        auto* chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
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

void Chat::slot_loading_chat_user() {
    if (_b_loading) {                                                                  // 防止在数据加载过程中重复触发加载函数,避免多次弹出加载对话框
        return;
    }

    _b_loading = true;
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);                                                     // 设置为模态对话框，阻塞用户对其他窗口的交互
    loadingDialog->show();                                                             // 显示对话框，但不会阻塞代码执行
    qDebug() << "add new data to list.....";
    AddChatUserList();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();                                                      // 在当前事件循环结束后才真正删除(对话框显示后立即开始加载，加载完成后立即删除)

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
