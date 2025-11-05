#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

// 前向声明，避免循环包含
class ShowWindow;

class Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    Widget(QWidget* parent=0);
    ~Widget(void);
    QPoint lastPos;
    QPoint pressPos;

public slots:
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    GLint numSteps=0;
    GLfloat rotationX=0.0;
    GLfloat rotationY=0.0;
    GLfloat rotationZ=0.0;
    GLfloat translateX=0.0;
    GLfloat translateY=0.0;

};

#endif // WIDGET_H
