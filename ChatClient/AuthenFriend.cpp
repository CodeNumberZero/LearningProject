#include "AuthenFriend.h"
#include "UserMgr.h"
#include "TcpMgr.h"

AuthenFriend::AuthenFriend(QWidget *parent) : QDialog(parent), _label_point(2, 6)
{
    ui.setupUi(this);
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    this->setObjectName("AuthenFriend");
    this->setModal(true);
    //ui.apply_lineEdit->setPlaceholderText(tr("nox"));                         // 设置搜索框中的默认文本
    ui.label_lineEdit->setPlaceholderText("搜索、添加标签");                         // 设置文本框中的默认文本
    ui.remark_lineEdit->setPlaceholderText("五彩斑斓的黑");
    ui.label_lineEdit->SetMaxLength(21);
    ui.label_lineEdit->move(2, 2);                                            // move的作用是设置控件在其父控件中的位置的函数;move(2, 2)将label_lineEdit控件移动到其父控件的 (2, 2) 坐标位置
    ui.label_lineEdit->setFixedHeight(20);
    ui.label_lineEdit->setMaxLength(10);
    ui.input_tip_wid->hide();

    _tip_cur_point = QPoint(5, 5);
    _tip_data = { "同学","家人","菜鸟教程","C++ Primer","Rust 程序设计",
                             "零一","zzz","极狐",
                                "哥查德","时王","微信读书","利维斯" };
    connect(ui.more_label, &ClickedOnceLabel::clicked, this, &AuthenFriend::ShowMoreLabel);
    InitTipLbs();

    // 链接输入标签回车事件
    connect(ui.label_lineEdit, &CustomizeEdit::returnPressed, this, &AuthenFriend::SlotLabelEnter);
    connect(ui.label_lineEdit, &CustomizeEdit::textChanged, this, &AuthenFriend::SlotLabelTextChange);
    connect(ui.label_lineEdit, &CustomizeEdit::editingFinished, this, &AuthenFriend::SlotLabelEditFinished);
    connect(ui.tip_label, &ClickedOnceLabel::clicked, this, &AuthenFriend::SlotAddFirendLabelByClickTip);

    ui.scrollArea->horizontalScrollBar()->setHidden(true);
    ui.scrollArea->verticalScrollBar()->setHidden(true);
    ui.sure_Button->SetState("normal", "hover", "press");
    ui.cancel_Button->SetState("normal", "hover", "press");
    ui.scrollArea->installEventFilter(this);                                       // 将当前对象安装为滚动区域的事件过滤器,让AuthenFriend对象监视滚动区域的所有事件,当有事件发生时,会先调用AuthenFriend::eventFilter函数
    /*
        1、安装事件过滤器:将一个对象(filterObj)安装为另一个对象的事件监视器; 被安装的对象的所有事件都会被filterObj先看到; filterObj可以在事件到达目标对象之前拦截和处理事件
        2、如果将当前对象安装为某个对象的事件过滤器,当前对象就必须重写eventFilter,因为installEventFilter只是注册了过滤器,但真正的过滤逻辑必须在eventFilter函数中实现;
           如果没有重写eventFilter,过滤器存在但没有实际作用,所有事件都不会被过滤，直接传递给目标对象
    */

    // 连接确认和取消按钮的槽函数
    connect(ui.cancel_Button, &QPushButton::clicked, this, &AuthenFriend::SlotAuthenCancel);
    connect(ui.sure_Button, &QPushButton::clicked, this, &AuthenFriend::SlotAuthenSure);
}

AuthenFriend::~AuthenFriend()
{
    qDebug() << "AuthenFriend Destructed!";
}

void AuthenFriend::InitTipLbs()
{
    int lines = 1;
    for (int i = 0; i < _tip_data.size(); i++) {
        auto* lb = new ClickedLabel(ui.label_list_wid);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);
        
        QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
        int textHeight = fontMetrics.height(); // 获取文本的高度
        if (_tip_cur_point.x() + textWidth + tip_offset > ui.label_list_wid->width()) {
            lines++;
            if (lines > 2) {
                delete lb;
                return;
            }
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }
        auto next_point = _tip_cur_point;
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point;
    }
}

// 将标签添加到展示区
void AuthenFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
    lb->move(cur_point);
    lb->show();
    _add_labels.insert(lb->text(), lb);
    _add_label_keys.push_back(lb->text());
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

// 重写事件过滤器展示滑动条
bool AuthenFriend::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui.scrollArea && event->type() == QEvent::Enter)
    {
        ui.scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if (obj == ui.scrollArea && event->type() == QEvent::Leave)
    {
        ui.scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}

// 后期搜索用户功能用户数据会从服务器传回来,实现相关接口
void AuthenFriend::SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    _apply_info = apply_info;
    ui.remark_lineEdit->setPlaceholderText(apply_info->_name);     // 设置备注框中的默认文本
}

// 重排好友标签编辑栏的标签
void AuthenFriend::resetLabels()
{
    auto max_width = ui.grid_wid->width();
    auto label_height = 0;
    for (auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++) {
        //todo... 添加宽度统计
        if (_label_point.x() + iter.value()->width() > max_width) {
            _label_point.setY(_label_point.y() + iter.value()->height() + 6);
            _label_point.setX(2);
        }
        iter.value()->move(_label_point);
        iter.value()->show();
        _label_point.setX(_label_point.x() + iter.value()->width() + 2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }
    if (_friend_labels.isEmpty()) {
        ui.label_lineEdit->move(_label_point);
        return;
    }
    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui.grid_wid->width()) {
        ui.label_lineEdit->move(2, _label_point.y() + label_height + 6);
    }
    else {
        ui.label_lineEdit->move(_label_point);
    }
}

// 添加好友标签编辑栏的标签
void AuthenFriend::addLabel(QString name)
{
    if (_friend_labels.find(name) != _friend_labels.end()) {
        ui.label_lineEdit->clear();
        return;
    }

    auto tmplabel = new FriendLabel(ui.grid_wid);
    tmplabel->SetText(name);
    tmplabel->setObjectName("FriendLabel");

    auto max_width = ui.grid_wid->width();
    //todo... 添加宽度统计
    if (_label_point.x() + tmplabel->width() > max_width) {
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(2);
    }
    else {

    }

    tmplabel->move(_label_point);
    tmplabel->show();
    _friend_labels[tmplabel->Text()] = tmplabel;
    _friend_label_keys.push_back(tmplabel->Text());

    connect(tmplabel, &FriendLabel::sig_close, this, &AuthenFriend::SlotRemoveFriendLabel);

    _label_point.setX(_label_point.x() + tmplabel->width() + 2);

    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui.grid_wid->width()) {
        ui.label_lineEdit->move(2, _label_point.y() + tmplabel->height() + 2);
    }
    else {
        ui.label_lineEdit->move(_label_point);
    }

    ui.label_lineEdit->clear();

    if (ui.grid_wid->height() < _label_point.y() + tmplabel->height() + 2) {
        ui.grid_wid->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
    }
}

// 点击按钮，可展示更多标签的功能。
void AuthenFriend::ShowMoreLabel() {
    qDebug() << "receive more label clicked";
    ui.more_label_wid->hide();
    ui.label_list_wid->setFixedWidth(325);
    _tip_cur_point = QPoint(5, 5);
    auto next_point = _tip_cur_point;
    int textWidth = 0;
    int textHeight = 0;
    // 重排现有的label
    for (auto& added_key : _add_label_keys) {
        auto added_lb = _add_labels[added_key];
        QFontMetrics fontMetrics(added_lb->font()); // 获取QLabel控件的字体信息
        textWidth = fontMetrics.horizontalAdvance(added_lb->text()); // 获取文本的宽度
        textHeight = fontMetrics.height(); // 获取文本的高度
        if (_tip_cur_point.x() + textWidth + tip_offset > ui.label_list_wid->width()) {
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }
        added_lb->move(_tip_cur_point);
        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());
        _tip_cur_point = next_point;
    }
    // 添加未添加的
    for (int i = 0; i < _tip_data.size(); i++) {
        auto iter = _add_labels.find(_tip_data[i]);
        if (iter != _add_labels.end()) {
            continue;
        }
        auto* lb = new ClickedLabel(ui.label_list_wid);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);
        QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
        int textHeight = fontMetrics.height(); // 获取文本的高度
        if (_tip_cur_point.x() + textWidth + tip_offset > ui.label_list_wid->width()) {
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }
        next_point = _tip_cur_point;
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point;
    }
    int diff_height = next_point.y() + textHeight + tip_offset - ui.label_list_wid->height();
    ui.label_list_wid->setFixedHeight(next_point.y() + textHeight + tip_offset);
    //qDebug()<<"after resize ui->lb_list size is " <<  ui->lb_list->size();
    ui.scrollContents->setFixedHeight(ui.scrollContents->height() + diff_height);
}

// 点击回车后，在好友标签编辑栏添加标签，在标签展示栏添加标签
void AuthenFriend::SlotLabelEnter()
{
    if (ui.label_lineEdit->text().isEmpty()) {
        return;
    }

    auto text = ui.label_lineEdit->text();

    addLabel(ui.label_lineEdit->text());

    ui.input_tip_wid->hide();

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    // 找到了就只需设置状态为选中即可
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    // 判断标签展示栏是否有该标签
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    //标签展示栏也增加一个标签, 并设置绿色选中
    auto* lb = new ClickedLabel(ui.label_lineEdit);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);
    qDebug() << "ui->lb_list->width() is " << ui.label_lineEdit->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度
    qDebug() << "textWidth is " << textWidth;

    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui.label_lineEdit->width()) {

        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui.label_lineEdit->height();
    ui.label_lineEdit->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui.scrollContents->setFixedHeight(ui.scrollContents->height() + diff_height);
}

// 当点击好友标签编辑栏的标签的关闭按钮时会调用下面的槽函数
void AuthenFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() << "receive close signal";
    _label_point.setX(2);
    _label_point.setY(6);
    auto find_iter = _friend_labels.find(name);
    if (find_iter == _friend_labels.end()) {
        return;
    }
    auto find_key = _friend_label_keys.end();
    for (auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end();
        iter++) {
        if (*iter == name) {
            find_key = iter;
            break;
        }
    }
    if (find_key != _friend_label_keys.end()) {
        _friend_label_keys.erase(find_key);
    }
    delete find_iter.value();
    _friend_labels.erase(find_iter);
    resetLabels();
    auto find_add = _add_labels.find(name);
    if (find_add == _add_labels.end()) {
        return;
    }
    find_add.value()->ResetNormalState();
}

// 点击已有标签添加或删除新联系人的标签(当点击标签展示栏的标签，可以实现标签添加和删除)
void AuthenFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext);
    if (find_iter == _add_labels.end()) {
        return;
    }

    if (state == ClickLbState::Selected) {
        // 编写添加逻辑
        addLabel(lbtext);
        return;
    }

    if (state == ClickLbState::Normal) {
        // 编写删除逻辑
        SlotRemoveFriendLabel(lbtext);
        return;
    }
}

// 当标签文本变化时，下面提示框的文本跟随变化
void AuthenFriend::SlotLabelTextChange(const QString& text)
{
    if (text.isEmpty()) {
        ui.tip_label->setText("");
        ui.input_tip_wid->hide();
        return;
    }
    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (iter == _tip_data.end()) {
        auto new_text = add_prefix + text;
        ui.tip_label->setText(new_text);
        ui.input_tip_wid->show();
        return;
    }
    ui.tip_label->setText(text);
    ui.input_tip_wid->show();
}

// 如果编辑完成，则隐藏编辑框
void AuthenFriend::SlotLabelEditFinished()
{
    ui.input_tip_wid->hide();
}

// 点击提示框，也会添加标签
void AuthenFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if (index != -1) {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //找到了就只需设置状态为选中即可
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    //判断标签展示栏是否有该标签
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    //标签展示栏也增加一个标签, 并设置绿色选中
    auto* lb = new ClickedLabel(ui.label_list_wid);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);
    qDebug() << "ui->lb_list->width() is " << ui.label_list_wid->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.horizontalAdvance(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度
    qDebug() << "textWidth is " << textWidth;

    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui.label_list_wid->width()) {

        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui.label_list_wid->height();
    ui.label_list_wid->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui.scrollContents->setFixedHeight(ui.scrollContents->height() + diff_height);
}

void AuthenFriend::SlotAuthenSure()
{
    qDebug() << "Slot Authen Sure ";
    // 添加发送逻辑
    QJsonObject jsonObj;
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["fromuid"] = uid;
    jsonObj["touid"] = _apply_info->_uid;
    QString back_name = "";
    if (ui.remark_lineEdit->text().isEmpty()) {
        back_name = ui.remark_lineEdit->placeholderText();
    }
    else {
        back_name = ui.remark_lineEdit->text();
    }
    jsonObj["back"] = back_name;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sigSendData(ReqId::ID_AUTH_FRIEND_REQ, jsonData);

    this->hide();
    deleteLater();
}

void AuthenFriend::SlotAuthenCancel()
{
    qDebug() << "Slot Authen Cancel";
    this->hide();
    deleteLater();
}