#pragma once
#include "global.h"
#include "UserData.h"
#include "LoadingDialog.h"

// ËÑË÷ÁÐ±íÀà
class SearchList : public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget* parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

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

};
