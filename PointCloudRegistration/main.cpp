#include <QApplication>
#include "ui/mainwindow.h"
#include "ElaApplication.h"

int main(int argc, char *argv[])
{
    // 设置应用程序信息
    QApplication::setOrganizationName("PointCloudLab");
    QApplication::setApplicationName("PointCloudRegistration");
    QApplication::setApplicationVersion("1.0.0");
    
    QApplication a(argc, argv);
    
    // 初始化ElaApplication
    ElaApplication* elaApp = ElaApplication::getInstance();
    elaApp->init();
    
    // 创建并显示主窗口
    MainWindow w;
    w.show();
    
    return a.exec();
}
