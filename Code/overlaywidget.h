#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QPoint>

class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget *parent = nullptr);

    void clearPoints();
    void setPoint(const QPoint &pt);
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void rectangleReady(const QPoint &p1, const QPoint &p2);

private:
    QPoint point1, point2;
    bool hasFirst = false;
    bool hasSecond = false;
};

#endif // OVERLAYWIDGET_H
