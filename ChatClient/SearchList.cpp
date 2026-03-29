#include "SearchList.h"
#include "TcpMgr.h"
#include "AddUserItem.h"
#include "FindSuccessDialog.h"
#include "CustomizeEdit.h"
#include "FindFailDialog.h"
#include "UserMgr.h"

SearchList::SearchList(QWidget* parent) : QListWidget(parent), _find_dlg(nullptr), _search_edit(nullptr), _send_pending(false)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);                     // 隐藏滚动条
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   
    /*
        1、安装事件过滤器:将一个对象(filterObj)安装为另一个对象的事件监视器; 被安装的对象的所有事件都会被filterObj先看到; filterObj可以在事件到达目标对象之前拦截和处理事件
        2、如果将当前对象安装为某个对象的事件过滤器,当前对象就必须重写eventFilter,因为installEventFilter只是注册了过滤器,但真正的过滤逻辑必须在eventFilter函数中实现;
           如果没有重写eventFilter,过滤器存在但没有实际作用,所有事件都不会被过滤，直接传递给目标对象
    */
    this->viewport()->installEventFilter(this);                                     // 将当前对象安装为视口的事件过滤器,让SearchList对象监视视口的所有事件,当有事件发生时,会先调用SearchList::eventFilter函数

    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked); // 连接点击的信号和槽；条目被点击时触发槽函数

    addTipItem();                                                                   // 添加条目

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigUserSearch, this, &SearchList::slot_user_search);     // 连接搜索条目
}

void SearchList::CloseFindDlg()
{
    if (_find_dlg) {
        _find_dlg->hide();
        _find_dlg = nullptr;
    }
}

void SearchList::SetSearchEdit(QWidget* edit)
{
    _search_edit = edit;
}

bool SearchList::eventFilter(QObject* watched, QEvent* event) {
    // 检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            // 鼠标悬浮，显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else if (event->type() == QEvent::Leave) {
            // 鼠标离开，隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    // 检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 计算滚动步数

        // 设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        return true; // 停止事件传递
    }
    return QListWidget::eventFilter(watched, event);
}

void SearchList::waitPending(bool pending)
{
    if (pending) {
        _loadingDialog = new LoadingDialog(this);
        _loadingDialog->setModal(true);                        // 设置为模态对话框，阻塞用户对其他窗口的交互
        _loadingDialog->show();                                // 显示对话框，但不会阻塞代码执行
        _send_pending = pending;
    }
    else {
        _loadingDialog->hide();                                // 隐藏对话框
        _loadingDialog->deleteLater();                         // 在当前事件循环结束后才真正删除(对话框显示后立即开始加载，加载完成后立即删除)
        _send_pending = pending;
    }
}

// 添加条目
void SearchList::addTipItem()
{
    auto* invalid_item = new QWidget();
    QListWidgetItem* item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250, 10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);

    auto* add_user_item = new AddUserItem();
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false);
    if (si == nullptr) {
        _find_dlg = std::make_shared<FindFailDialog>(this);
    }
    else {
        // 此处分两种情况:一种是搜索到已经是自己的朋友了,一种是未添加好友
        // 查找是否已经是好友
        bool b_exist = UserMgr::GetInstance()->CheckFriendById(si->_uid);
        if (b_exist) {
            // 此处处理已经添加的好友,实现页面跳转
            // 跳转到聊天界面指定的item中
            emit sigJumpChatItem(si);
            return;
        }
        // 此处先处理为添加的好友
        _find_dlg = std::make_shared<FindSuccessDialog>(this);
        std::dynamic_pointer_cast<FindSuccessDialog>(_find_dlg)->SetSearchInfo(si);
    }
    _find_dlg->show();
}

void SearchList::slot_item_clicked(QListWidgetItem* item) {
    QWidget* widget = this->itemWidget(item);                               // 获取自定义widget对象
    if (!widget) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == ListItemType::INVALID_ITEM) {
        qDebug() << "slot invalid item clicked ";
        return;
    }

    if (itemType == ListItemType::ADD_USER_TIP_ITEM) {
        if (_send_pending) {                                                                 // 搜索好友的时候网络会有延迟,为了防止在搜索过程中重复触发加载函数,避免多次弹出加载对话框 用一个变量来控制是否加载页面(加载界面即LoadingDialog页面,图形为转圈的图片)
            return;
        }

        if (!_search_edit) {
            return;
        }

        waitPending(true);
        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        auto uid_str = search_edit->text();      
        //此处发送请求给server
        QJsonObject jsonObj;
        jsonObj["uid"] = uid_str;                                                            // 这里虽然变量名用的是uid,但是用户在search_lineedit输入信息搜索用户时可能输入的是uid,也可能是name(见Chat.cpp第92行).因此后续服务器在处理逻辑时要先判断是uid还是name,根据不同类型分开处理(见ChatServer项目LogicSystem.cpp文件SearchInfo函数的150行)

        QJsonDocument doc(jsonObj);
        //QString jsonString = doc.toJson(QJsonDocument::Indented);                            
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);                             // 紧凑格式，没有多余空格(Indented格式有缩进，可读性高）)

        // 发送tcp请求给chat server
        emit TcpMgr::GetInstance()->sigSendData(ReqId::ID_SEARCH_USER_REQ, jsonData);

        //_find_dlg = std::make_shared<FindSuccessDialog>(this);                               // 有成功也有失败，用基类指针承接，需要判断是否成功时再转换成具体的派生类
        //auto si = std::make_shared<SearchInfo>(0, "远古织影者", "远古织影者", "hello , my friend!", 0);
        //(std::dynamic_pointer_cast<FindSuccessDialog>(_find_dlg))->SetSearchInfo(si);        // 需要调用派生类特有的 SetSearchInfo 方法，所以必须向下转型
        //_find_dlg->show();

        return;
    }

    // 清除弹出框
    CloseFindDlg();
}