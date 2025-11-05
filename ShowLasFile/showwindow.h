#ifndef SHOWWINDOW_H
#define SHOWWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "lasreader.h"

// 前向声明
class Widget;

namespace Ui {
class ShowWindow;
}

class ShowWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ShowWindow(QWidget *parent = nullptr);
    ~ShowWindow();
    bool loadlasFile(const QString &fileName);
    void DrawLasPoints();

private slots:
    void on_openBtn_clicked();
    void on_CloseBtn_clicked();

private:
    Ui::ShowWindow *ui;
    QString fileName;
    PointCloud pointCloud;  // 使用自定义的PointCloud
};

#endif // SHOWWINDOW_H
