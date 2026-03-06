#include "ChatItemBase.h"

// 注意：在声明中已经给了parent的默认值，定义中就不需要再写了，否则会报错
ChatItemBase::ChatItemBase(ChatRole role, QWidget* parent) : QWidget(parent), m_role(role)
{
    m_pNameLabel = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHei");
    font.setPointSize(9);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(20);                                                // 设置用户名标签

    m_pIconLabel = new QLabel();          
    m_pIconLabel->setScaledContents(true);
    m_pIconLabel->setFixedSize(42, 42);                                              // 设置头像标签

    m_pBubble = new QWidget();                                                       // 设置气泡窗口

    QGridLayout* pGLayout = new QGridLayout();                                       // 设置布局，创建网格布局
    pGLayout->setVerticalSpacing(3);
    pGLayout->setHorizontalSpacing(3);
    pGLayout->setContentsMargins(3, 3, 3, 3);

    QSpacerItem* pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum); // 创建伸缩空间(即弹簧)
    if (m_role == ChatRole::Self)                                                    // 根据角色动态调整布局，自己发送的消息(右侧显示)
    {
        m_pNameLabel->setContentsMargins(0, 0, 8, 0);
        m_pNameLabel->setAlignment(Qt::AlignRight);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
        pGLayout->addWidget(m_pIconLabel, 0, 2, 2, 1, Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1);
        pGLayout->setColumnStretch(0, 2);
        pGLayout->setColumnStretch(1, 3);
    }
    else {                                                                           // 对方发送的消息(左侧显示)
        m_pNameLabel->setContentsMargins(8, 0, 0, 0);
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2, 1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1);
        pGLayout->addItem(pSpacer, 2, 2, 1, 1);
        pGLayout->setColumnStretch(1, 3);
        pGLayout->setColumnStretch(2, 2);
    }
    this->setLayout(pGLayout);
}

// 设置用户名
void ChatItemBase::setUserName(const QString& name)
{
    m_pNameLabel->setText(name);
}

// 设置头像
void ChatItemBase::setUserIcon(const QPixmap& icon)
{
    m_pIconLabel->setPixmap(icon);
}

// 因为还要定制化实现气泡widget，所以要写个函数更新这个widget
void ChatItemBase::setWidget(QWidget* w)
{
    QGridLayout* pGLayout = (qobject_cast<QGridLayout*>)(this->layout());
    pGLayout->replaceWidget(m_pBubble, w);                                           // 用新的Widget替换旧的
    delete m_pBubble;                                                                // 回收旧的Widget占用的空间        
    m_pBubble = w;                                                                   // 将新的Widget的指针赋给原指针
}
