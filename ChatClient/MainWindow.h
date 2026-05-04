#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "Login.h"
#include "Register.h"
#include "resetdialog.h"
#include "Chat.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchReset();
    void SlotSwitchLoginFromReset();
    void SlotSwitchChat();
    void SlotOffLine();

private:
    Ui::MainWindowClass ui;
    Login* _login;
    Register* _register;
    ResetDialog* _reset;
    Chat* _chat;

    void OffLineLogin();
};

