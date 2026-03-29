#pragma once
#include "global.h"
#include "UserData.h"
#include "LoadingDialog.h"

// 搜索列表类
class SearchList : public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget* parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;  // 返回值含义：true表示事件已处理，不再传递给目标对象；false表示事件继续正常传递

private:
    bool _send_pending;
    std::shared_ptr<QDialog> _find_dlg;
    QWidget* _search_edit;
    LoadingDialog* _loadingDialog;
    void waitPending(bool pending = true);
    void addTipItem();
private slots:
    void slot_item_clicked(QListWidgetItem* item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);
signals:
    void sigJumpChatItem(std::shared_ptr<SearchInfo> si);
};
