#include "drawwidget.h"
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QDebug>

DrawWidget::DrawWidget(QWidget *parent) : QWidget(parent) {
    // 确保透明背景，避免覆盖整个控件区域
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    // 确保接收鼠标事件
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
}


void DrawWidget::loadPointsFromFile(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file";
        return;
    }

    points.clear();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(" 坐标:");
        if (parts.size() == 2) {
            QStringList coords = parts[1].split(",");
            if (coords.size() == 2) {
                bool ok1, ok2;
                double lon = coords[0].toDouble(&ok1);
                double lat = coords[1].toDouble(&ok2);
                if (ok1 && ok2) {
                    // 进行坐标转换
                    double windowX = (lon - 116.0047826086956) / 0.0013043478261;
                    double windowY = (lat - 40.16189376443418) / -0.00092378752886;
                    QPointF point(windowX, windowY);
                    points.append(point);

                    // 打印转换后的坐标，检查是否合理
                    qDebug() << "Converted point: (" << windowX << ", " << windowY << ")";
                }
            }
        }
    }
}

void DrawWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setPen(Qt::black);

    // 如果没有点要绘制，返回
    if (points.isEmpty()) {
        qDebug() << "No points to draw!";
        return;
    }

    painter.setBrush(QBrush(Qt::red));
    painter.setPen(Qt::NoPen);

    for (const QPointF &pt : points) {
        // 在每个点的位置画一个半径为 2 的红色实心圆
        painter.drawEllipse(pt, 2.0, 2.0);
    }
}
