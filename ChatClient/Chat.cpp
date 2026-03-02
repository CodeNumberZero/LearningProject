#include "Chat.h"
#include "ChatUserWid.h"

Chat::Chat(QWidget *parent)
	: QDialog(parent), _mode(ChatUIMode::ChatMode), _state(ChatUIMode::ChatMode), _b_loading(false)
{
	ui.setupUi(this);
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
    AddChatUserList();
}

Chat::~Chat()
{}

// 定义一些全局的变量用来做测试
std::vector<QString>  strs = { "hello world !",
                             "nice to meet u",
                             "New year，new life",
                            "You have to love yourself",
                            "My love is written in the wind ever since the whole world is you" };

std::vector<QString> heads = {
    ":/image/resource/head_1.jpg",
    ":/image/resource/head_6.jpg",
    ":/image/resource/head_7.jpg",
    ":/image/resource/head_19.jpg",
    ":/image/resource/head_6.jpg"
};

std::vector<QString> names = {
    "zero-one",
    "saber",
    "revice",
    "geat",
    "gavv",
    "zzz",
    "python",
    "rust"
};

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

