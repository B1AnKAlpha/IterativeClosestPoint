#include "widget.h"
#include "showwindow.h"
#include <QtMath>

// 全局变量定义
PointCloud g_pointCloud;
float max_x = -10000, min_x = 10000;
float max_y = -10000, min_y = 10000;
float max_z = -10000, min_z = 10000;

Widget::Widget(QWidget* parent):QOpenGLWidget(parent)
{
}

Widget::~Widget(void)
{
}

void Widget::initializeGL()
{
    initializeOpenGLFunctions();  // 初始化OpenGL函数
    glClearColor( 1.0, 1.0, 1.0, 0.0 );
    glShadeModel(GL_SMOOTH);
    glClearDepth( 1.0 );
    glEnable( GL_DEPTH_TEST );  
    glViewport(0, 0,this->width(),this->height());

    //光照
    float intensity[] = {1,1,1,1};
    float position[] = {1,1,5,0};
    glLightfv(GL_LIGHT0,GL_DIFFUSE,intensity);
    glLightfv(GL_LIGHT0,GL_SPECULAR,intensity);
    glLightfv(GL_LIGHT0,GL_POSITION,position);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
}

void Widget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );//zuobiao->0  
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
}

void Widget::paintGL()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // 手动实现透视投影，替代gluPerspective
    GLfloat fovy = 80.0f;
    GLfloat aspect = (GLfloat)this->width() / (GLfloat)this->height();
    GLfloat zNear = 0.0625f;
    GLfloat zFar = 1500.0f;
    
    GLfloat fH = tan(fovy / 360.0f * M_PI) * zNear;
    GLfloat fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(translateX,translateY,-50.0f+numSteps);//沿XYZ坐标轴平移
    glRotatef(180+rotationX, 1.0, 0.0, 0.0);//沿X轴旋转
    glRotatef(rotationY, 0.0, 1.0, 0.0);
    glRotatef(rotationZ, 0.0, 0.0, 1.0);

    ShowWindow showwindow;
    showwindow.DrawLasPoints();
}

void Widget::wheelEvent(QWheelEvent *event)//滑轮缩放
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    int numDegrees = event->angleDelta().y() / 8;
#else
    int numDegrees = event->delta() / 8;
#endif
    numSteps += numDegrees / 15;
    update();  // Qt 6使用update()替代updateGL()
}

void Widget::mousePressEvent(QMouseEvent *event)//鼠标按住
{
    lastPos = event->pos();
    if(Qt::LeftButton == event->button())
    {
        pressPos = event->pos();
        update();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)//鼠标移动
{
    QPointF pos = event->position();
    GLfloat dx = GLfloat(lastPos.x() - pos.x()) / 1280;
    GLfloat dy = GLfloat(pos.y() - lastPos.y()) / 640;
    
    if (event->buttons() & Qt::LeftButton)
    {
        rotationX += 180 * dy;
        rotationY += 180 * dx;
    }
    else if (event->buttons() & Qt::RightButton)
    {
        translateX -= 100*dx;
        translateY -= 100*dy;
    }
    lastPos = pos.toPoint();
    update();
}
