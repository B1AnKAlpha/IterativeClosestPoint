#ifndef DRAWWIDGET_H
#define DRAWWIDGET_H

#include <QWidget>
#include <QPointF>
#include <QVector>

class DrawWidget : public QWidget {
    Q_OBJECT

public:
    explicit DrawWidget(QWidget *parent = nullptr);
    void loadPointsFromFile(const QString &filename); // 加载点坐标

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPointF> points; // 存储窗口坐标点
};

#endif // DRAWWIDGET_H
