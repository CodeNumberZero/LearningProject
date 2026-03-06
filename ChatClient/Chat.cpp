#include "Chat.h"
#include "ChatUserWid.h"
#include "LoadingDialog.h"

// 
/*
    定义一些全局的变量用来做测试；
    这些变量不能放到global.h中，因为globla.h在多个文件中被包含，变量放进去可能会出现重定义的问题；
    例如BubbleFrame.h和TextBubble.h都包含了global.h，而TextBubble.h又包含了BubbleFrame.h，这种出现了TextBubble.h包含两次global.h,导致在链接时出错
*/
const std::vector<QString>  strs = { "hello world !",
                             "nice to meet u",
                             "New year，new life",
                            "You have to love yourself",
                            "My love is written in the wind ever since the whole world is you" };

const std::vector<QString> heads = {
    ":/image/resource/head_1.jpg",
    ":/image/resource/head_6.jpg",
    ":/image/resource/head_7.jpg",
    ":/image/resource/head_19.jpg",
    ":/image/resource/head_6.jpg"
};

const std::vector<QString> names = {
    "zero-one",
    "saber",
    "revice",
    "geat",
    "gavv",
    "zzz",
    "python",
    "rust"
};

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
	ui.search_lineEdit->SetMaxLength(50);                                          // 设置搜索框可输入的最大字节数

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
        clearAction->setIcon(QIcon(":/res/close_transparent.png"));               // 清除文本后，切换回透明图标
        ui.search_lineEdit->clearFocus();                                         // 清除焦点
        //清除按钮被按下则不显示搜索框
        ShowSearch(false);
    });
    
    ShowSearch(false);                                                            // 默认情况下也不显示搜索框

    connect(ui.chat_user_list, &ChatUserList::sig_loading_chat_user, this, &Chat::slot_loading_chat_user);

    AddChatUserList();
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