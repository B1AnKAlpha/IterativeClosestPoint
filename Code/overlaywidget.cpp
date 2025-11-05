#include "overlaywidget.h"
#include <QPainter>
#include <QMouseEvent>

OverlayWidget::OverlayWidget(QWidget *parent) : QWidget(parent) {
   // setAttribute(Qt::WA_TransparentForMouseEvents, true); // 接收鼠标事件
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false); // 要接收鼠标事件
}

void OverlayWidget::clearPoints() {
    hasFirst = false;
    hasSecond = false;
    update();
}

void OverlayWidget::setPoint(const QPoint &pt) {
    if (!hasFirst) {
        point1 = pt;
        hasFirst = true;
        hasSecond = false;
    } else {
        point2 = pt;
        hasSecond = true;
        emit rectangleReady(point1, point2);
    }
    update();
}

void OverlayWidget::mousePressEvent(QMouseEvent *event) {
    if (hasFirst && hasSecond) {
        // 第三次点击，重置
        clearPoints();
    }
    setPoint(event->pos());
}

void OverlayWidget::paintEvent(QPaintEvent *) {
    if (hasFirst && hasSecond) {
        QPainter painter(this);
        painter.setPen(QPen(Qt::red, 2));
        QRect rect(QPoint(std::min(point1.x(), point2.x()), std::min(point1.y(), point2.y())),
                   QPoint(std::max(point1.x(), point2.x()), std::max(point1.y(), point2.y())));
        painter.drawRect(rect);
    }
}
