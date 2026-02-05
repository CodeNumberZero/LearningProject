#include "MainWindow.h"
#include "Login.h"
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile qss(":/qss/style/stylesheet.qss");
    if (qss.open(QFile::ReadOnly)) {
        qDebug("Open success!");
        QString style = QLatin1String(qss.readAll());
        //qDebug() << style;
        //app.setStyleSheet(qss.readAll());
        app.setStyleSheet(style);
        qss.close();
    }
    else {
        qDebug("Open failed!");
    }

    QString app_path = QCoreApplication::applicationDirPath();                                 // 获取当前可执行文件(.exe)所在的绝对路径(不是源码路径,而是运行时的输出目录),本例中是"D:\Documents\VisualStudioFiles\project\ChatProject\x64\Debug\ChatClient.exe"
    QString fileName = "ClientConfig.ini";                 
    QString config_path = QDir::toNativeSeparators(app_path +QDir::separator() + fileName);    // 拼接文件名.QDir::separator()获取系统对应的路径分隔符(Windows是\,Linux/macOS是/),避免手动写\或/导致跨平台兼容问题;QDir::toNativeSeparators()将拼接后的路径转换为当前系统的原生格式,比如把/转为\,确保路径在不同系统下都能被正确识别
    QSettings settings(config_path, QSettings::IniFormat);                                     // 创建QSettings对象,指定要读取的INI文件路径和格式.QSettings是Qt提供的配置文件读写工具,支持INI、注册表(Windows)、plist(macOS)等格式;IniFormat参数明确告诉QSettings按INI格式解析文件
    QString gate_host = settings.value("GateServer/Host").toString();                          // 读取INI文件中[GateServer]节下的host和port值,并转为QString类型;若配置文件中没有host或port,会返回空字符串
    QString gate_port = settings.value("GateServer/Port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;
    qDebug() << gate_url_prefix;

    MainWindow window;
    window.show();

    return app.exec();
}

/*
在Visual Studio中配置自动复制config.ini(因为程序最终的可执行文件.exe会输出到其他目录,所以需要将配置也拷贝到.exe所在的目录;也可以不拷贝,直接将配置文件添加到qrc资源文件中,读取配置时使用qrc文件中的配置就行):
    1、右键项目(ChatClient) -> 选择「属性」
    2、在左侧菜单里找到"生成事件" -> "后期生成事件"
    3、在"命令行"输入框里,粘贴下面的命令:copy /Y "$(ProjectDir)config.ini" "$(OutDir)"
        $(ProjectDir)：表示项目根目录(config.ini所在的位置)
        $(OutDir)：表示编译后的输出目录(Debug/Release,对应Qt Creator的bin目录,即config.ini复制后存放的目录)
        /Y：覆盖目标文件时不提示确认。
    4、点击"确定"保存设置。
*/