#include "pointcloudviewer.h"
#include <QOpenGLContext>
#include <cmath>

PointCloudViewer::PointCloudViewer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_sourceCloud(nullptr)
    , m_targetCloud(nullptr)
    , m_originalSource(nullptr)
    , m_currentIteration(-1)
    , m_cameraPos(0, 0, 5)
    , m_cameraTarget(0, 0, 0)
    , m_cameraUp(0, 1, 0)
    , m_isRotating(false)
    , m_isPanning(false)
    , m_rotationX(0)
    , m_rotationY(0)
    , m_zoom(1.0f)
    , m_panOffset(0, 0, 0)
    , m_showGrid(true)
    , m_showAxes(true)
    , m_smoothRendering(true)
    , m_sourceColor(255, 100, 100)
    , m_targetColor(100, 100, 255)
    , m_pointSize(2.0f)
    , m_sceneCenter(0, 0, 0)
    , m_sceneRadius(1.0f)
{
    setFocusPolicy(Qt::StrongFocus);
}

PointCloudViewer::~PointCloudViewer()
{
    if (m_originalSource) {
        delete m_originalSource;
    }
}

void PointCloudViewer::setSourceCloud(PointCloud* cloud)
{
    m_sourceCloud = cloud;
    
    // 保存原始源点云副本用于迭代回放
    if (m_originalSource) {
        delete m_originalSource;
    }
    if (cloud && !cloud->empty()) {
        m_originalSource = new PointCloud();
        m_originalSource->points = cloud->points;
        m_originalSource->color = cloud->color;
        m_originalSource->computeBounds();
    } else {
        m_originalSource = nullptr;
    }
    
    fitToScreen();
    update();
}

void PointCloudViewer::setTargetCloud(PointCloud* cloud)
{
    m_targetCloud = cloud;
    fitToScreen();
    update();
}

void PointCloudViewer::clearClouds()
{
    m_sourceCloud = nullptr;
    m_targetCloud = nullptr;
    if (m_originalSource) {
        delete m_originalSource;
        m_originalSource = nullptr;
    }
    m_iterationHistory.clear();
    m_currentIteration = -1;
    update();
}

void PointCloudViewer::setIterationHistory(const std::vector<IterationResult>& history)
{
    m_iterationHistory = history;
    m_currentIteration = -1;
}

void PointCloudViewer::setCurrentIteration(int index)
{
    if (index < -1 || index >= static_cast<int>(m_iterationHistory.size())) {
        return;
    }
    
    m_currentIteration = index;
    
    // 应用变换到源点云
    if (m_sourceCloud && m_originalSource && !m_originalSource->empty()) {
        m_sourceCloud->points = m_originalSource->points;
        
        if (index >= 0 && index < static_cast<int>(m_iterationHistory.size())) {
            const auto& transform = m_iterationHistory[index].transform;
            
            // 转换Eigen矩阵为数组格式
            double R[3][3], t[3];
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    R[i][j] = transform(i, j);
                }
                t[i] = transform(i, 3);
            }
            
            m_sourceCloud->applyTransform(R, t);
        }
    }
    
    emit iterationChanged(index);
    update();
}

void PointCloudViewer::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void PointCloudViewer::setShowAxes(bool show)
{
    m_showAxes = show;
    update();
}

void PointCloudViewer::setSmoothRendering(bool smooth)
{
    m_smoothRendering = smooth;
    update();
}

void PointCloudViewer::setSourceColor(const QColor& color)
{
    m_sourceColor = color;
    update();
}

void PointCloudViewer::setTargetColor(const QColor& color)
{
    m_targetColor = color;
    update();
}

void PointCloudViewer::setPointSize(float size)
{
    m_pointSize = size;
    update();
}

void PointCloudViewer::resetView()
{
    m_rotationX = 0;
    m_rotationY = 0;
    m_zoom = 1.0f;
    m_panOffset = QVector3D(0, 0, 0);
    updateCamera();
    update();
}

void PointCloudViewer::fitToScreen()
{
    // 计算场景边界
    if (!m_sourceCloud && !m_targetCloud) {
        m_sceneCenter = QVector3D(0, 0, 0);
        m_sceneRadius = 1.0f;
        return;
    }
    
    double minX = 1e10, maxX = -1e10;
    double minY = 1e10, maxY = -1e10;
    double minZ = 1e10, maxZ = -1e10;
    
    if (m_sourceCloud && !m_sourceCloud->empty()) {
        const_cast<PointCloud*>(m_sourceCloud)->computeBounds();
        minX = std::min(minX, m_sourceCloud->minX);
        maxX = std::max(maxX, m_sourceCloud->maxX);
        minY = std::min(minY, m_sourceCloud->minY);
        maxY = std::max(maxY, m_sourceCloud->maxY);
        minZ = std::min(minZ, m_sourceCloud->minZ);
        maxZ = std::max(maxZ, m_sourceCloud->maxZ);
    }
    
    if (m_targetCloud && !m_targetCloud->empty()) {
        const_cast<PointCloud*>(m_targetCloud)->computeBounds();
        minX = std::min(minX, m_targetCloud->minX);
        maxX = std::max(maxX, m_targetCloud->maxX);
        minY = std::min(minY, m_targetCloud->minY);
        maxY = std::max(maxY, m_targetCloud->maxY);
        minZ = std::min(minZ, m_targetCloud->minZ);
        maxZ = std::max(maxZ, m_targetCloud->maxZ);
    }
    
    m_sceneCenter = QVector3D(
        static_cast<float>((minX + maxX) / 2.0),
        static_cast<float>((minY + maxY) / 2.0),
        static_cast<float>((minZ + maxZ) / 2.0)
    );
    
    double dx = maxX - minX;
    double dy = maxY - minY;
    double dz = maxZ - minZ;
    m_sceneRadius = static_cast<float>(std::sqrt(dx*dx + dy*dy + dz*dz) / 2.0);
    
    // 重置视图
    resetView();
}

void PointCloudViewer::initializeGL()
{
    initializeOpenGLFunctions();
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    
    if (m_smoothRendering) {
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void PointCloudViewer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    
    m_projection.setToIdentity();
    float aspect = static_cast<float>(w) / static_cast<float>(h > 0 ? h : 1);
    m_projection.perspective(45.0f, aspect, 0.1f, 1000.0f);
}

void PointCloudViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    updateCamera();
    
    // 绘制网格
    if (m_showGrid) {
        drawGrid();
    }
    
    // 绘制坐标轴
    if (m_showAxes) {
        drawAxes();
    }
    
    // 设置点大小
    glPointSize(m_pointSize);
    
    // 绘制目标点云（蓝色）
    if (m_targetCloud && !m_targetCloud->empty()) {
        drawPointCloud(m_targetCloud, m_targetColor);
    }
    
    // 绘制源点云（红色）
    if (m_sourceCloud && !m_sourceCloud->empty()) {
        drawPointCloud(m_sourceCloud, m_sourceColor);
    }
}

void PointCloudViewer::drawGrid()
{
    glDisable(GL_DEPTH_TEST);
    
    QMatrix4x4 mvp = m_projection * m_view;
    
    glLoadMatrixf(mvp.constData());
    
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES);
    
    float gridSize = m_sceneRadius * 2.0f;
    int gridLines = 10;
    float step = gridSize / gridLines;
    
    for (int i = -gridLines; i <= gridLines; i++) {
        float pos = i * step;
        // X方向线
        glVertex3f(m_sceneCenter.x() + pos, m_sceneCenter.y() - gridSize, m_sceneCenter.z());
        glVertex3f(m_sceneCenter.x() + pos, m_sceneCenter.y() - gridSize, m_sceneCenter.z() + gridSize);
        
        // Z方向线
        glVertex3f(m_sceneCenter.x() - gridSize, m_sceneCenter.y() - gridSize, m_sceneCenter.z() + pos);
        glVertex3f(m_sceneCenter.x(), m_sceneCenter.y() - gridSize, m_sceneCenter.z() + pos);
    }
    
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

void PointCloudViewer::drawAxes()
{
    QMatrix4x4 mvp = m_projection * m_view;
    glLoadMatrixf(mvp.constData());
    
    float axisLength = m_sceneRadius * 0.3f;
    
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    
    // X轴 - 红色
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(m_sceneCenter.x(), m_sceneCenter.y(), m_sceneCenter.z());
    glVertex3f(m_sceneCenter.x() + axisLength, m_sceneCenter.y(), m_sceneCenter.z());
    
    // Y轴 - 绿色
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(m_sceneCenter.x(), m_sceneCenter.y(), m_sceneCenter.z());
    glVertex3f(m_sceneCenter.x(), m_sceneCenter.y() + axisLength, m_sceneCenter.z());
    
    // Z轴 - 蓝色
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(m_sceneCenter.x(), m_sceneCenter.y(), m_sceneCenter.z());
    glVertex3f(m_sceneCenter.x(), m_sceneCenter.y(), m_sceneCenter.z() + axisLength);
    
    glEnd();
    glLineWidth(1.0f);
}

void PointCloudViewer::drawPointCloud(const PointCloud* cloud, const QColor& color)
{
    if (!cloud || cloud->empty()) return;
    
    QMatrix4x4 mvp = m_projection * m_view;
    glLoadMatrixf(mvp.constData());
    
    glColor3f(color.redF(), color.greenF(), color.blueF());
    
    glBegin(GL_POINTS);
    for (const auto& p : cloud->points) {
        glVertex3d(p.x, p.y, p.z);
    }
    glEnd();
}

void PointCloudViewer::updateCamera()
{
    float distance = m_sceneRadius * 3.0f / m_zoom;
    
    m_view.setToIdentity();
    
    // 先移动到观察位置
    m_view.translate(0, 0, -distance);
    
    // 应用旋转(围绕场景中心)
    m_view.rotate(m_rotationX, 1, 0, 0);
    m_view.rotate(m_rotationY, 0, 1, 0);
    
    // 应用平移偏移
    m_view.translate(-m_sceneCenter.x() - m_panOffset.x(), 
                     -m_sceneCenter.y() - m_panOffset.y(), 
                     -m_sceneCenter.z() - m_panOffset.z());
}

void PointCloudViewer::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    
    if (event->button() == Qt::LeftButton) {
        m_isRotating = true;
    } else if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    }
}

void PointCloudViewer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();
    
    if (m_isRotating) {
        m_rotationY += delta.x() * 0.5f;
        m_rotationX += delta.y() * 0.5f;
        
        // 限制X轴旋转范围
        if (m_rotationX > 89.0f) m_rotationX = 89.0f;
        if (m_rotationX < -89.0f) m_rotationX = -89.0f;
        
        update();
    } else if (m_isPanning) {
        float panSpeed = m_sceneRadius * 0.001f;
        m_panOffset.setX(m_panOffset.x() + delta.x() * panSpeed);
        m_panOffset.setY(m_panOffset.y() - delta.y() * panSpeed);
        update();
    }
}

void PointCloudViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isRotating = false;
    } else if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
}

void PointCloudViewer::wheelEvent(QWheelEvent *event)
{
    float zoomDelta = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    m_zoom *= zoomDelta;
    
    // 限制缩放范围
    if (m_zoom < 0.1f) m_zoom = 0.1f;
    if (m_zoom > 10.0f) m_zoom = 10.0f;
    
    update();
}
