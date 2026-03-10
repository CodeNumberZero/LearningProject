#pragma once

#include <QWidget>
#include "ui_FriendLabel.h"
#include "global.h"

class FriendLabel : public QFrame
{
	Q_OBJECT

public:
	FriendLabel(QWidget *parent = nullptr);
	~FriendLabel();
    void SetText(QString text);
    int Width();
    int Height();
    QString Text();

private:
	Ui::FriendLabelClass ui;
    QString _text;
    int _width;
    int _height;

public slots:
    void slot_close();
signals:
    void sig_close(QString);
};

