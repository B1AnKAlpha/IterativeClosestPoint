#include "showwindow.h"
#include "ui_showwindow.h"
#include "widget.h"
#include <QMessageBox>

// 全局变量
extern PointCloud g_pointCloud;
extern float max_x, min_x, max_y, min_y, max_z, min_z;

ShowWindow::ShowWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ShowWindow)
{
    ui->setupUi(this);
}

ShowWindow::~ShowWindow()
{
    delete ui;
}

bool ShowWindow::loadlasFile(const QString &fileName)
{
    std::string filename = fileName.toStdString();
    
    // 使用采样率加快加载速度，根据文件大小可以调整
    int sample_rate = 10;  // 每10个点取1个
    
    if (!LASReader::readLASFile(filename, pointCloud, sample_rate)) {
        return false;
    }
    
    // 计算边界（如果需要的话）
    pointCloud.calculateBounds();
    
    // 更新全局变量用于渲染
    g_pointCloud = pointCloud;
    max_x = pointCloud.x_max;
    min_x = pointCloud.x_min;
    max_y = pointCloud.y_max;
    min_y = pointCloud.y_min;
    max_z = pointCloud.z_max;
    min_z = pointCloud.z_min;
    
    std::cout << "点云边界: " << std::endl;
    std::cout << "  X: [" << min_x << ", " << max_x << "]" << std::endl;
    std::cout << "  Y: [" << min_y << ", " << max_y << "]" << std::endl;
    std::cout << "  Z: [" << min_z << ", " << max_z << "]" << std::endl;
    
    return true;
}

void ShowWindow::on_openBtn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
        tr("打开LAS点云文件"), "", tr("LAS Files (*.las);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        if (loadlasFile(fileName)) {
            QMessageBox::information(this, tr("成功"), 
                tr("点云加载成功！\n点数: %1").arg(pointCloud.size()));
            ui->widget->update();  // 触发重绘
        } else {
            QMessageBox::critical(this, tr("错误"), tr("无法加载点云文件！"));
        }
    }
}

void ShowWindow::DrawLasPoints()
{
    if (g_pointCloud.points.empty()) return;
    
    // 计算中心点
    float center_x = (max_x + min_x) / 2.0f;
    float center_y = (max_y + min_y) / 2.0f;
    float center_z = (max_z + min_z) / 2.0f;
    
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    
    float boundingBoxZ = max_z - min_z;
    
    for (size_t i = 0; i < g_pointCloud.points.size(); i++)
    {
        const Point3D& p = g_pointCloud.points[i];
        float highZ = p.z - min_z;
        
        // 根据高度着色
        if (highZ < (boundingBoxZ / 3))
        {
            glColor3f(0.1f + 0.7f * highZ / (boundingBoxZ / 3), 0.1f, 0.1f);
        }
        else if (highZ < 2 * (boundingBoxZ / 3))
        {
            glColor3f(0.8f, 0.7f * (highZ - (boundingBoxZ / 3)) / (boundingBoxZ / 3), 0.1f);
        }
        else
        {
            glColor3f(0.8f, 0.8f, 0.1f + 0.7f * (highZ - 2 * (boundingBoxZ / 3)) / (boundingBoxZ / 3));
        }
        
        // 绘制点，以中心点为原点
        glVertex3f(p.x - center_x, p.y - center_y, p.z - center_z);
    }
    
    glEnd();
}

void ShowWindow::on_CloseBtn_clicked()
{
    this->close();
}
