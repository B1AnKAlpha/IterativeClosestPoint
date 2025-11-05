#ifndef POINTCLOUDVIEWER_H
#define POINTCLOUDVIEWER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include "core/pointcloud.h"
#include "core/icpengine.h"

/**
 * @brief OpenGL点云查看器
 * 
 * 支持3D点云显示、交互控制和迭代回放
 */
class PointCloudViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit PointCloudViewer(QWidget *parent = nullptr);
    ~PointCloudViewer() override;
    
    // 点云数据
    void setSourceCloud(PointCloud* cloud);
    void setTargetCloud(PointCloud* cloud);
    void clearClouds();
    
    // 迭代历史
    void setIterationHistory(const std::vector<IterationResult>& history);
    void setCurrentIteration(int index);
    int getCurrentIteration() const { return m_currentIteration; }
    int getIterationCount() const { return static_cast<int>(m_iterationHistory.size()); }
    
    // 显示设置
    void setShowGrid(bool show);
    void setShowAxes(bool show);
    void setSmoothRendering(bool smooth);
    void setSourceColor(const QColor& color);
    void setTargetColor(const QColor& color);
    void setPointSize(float size);
    
    // 视图控制
    void resetView();
    void fitToScreen();
    
signals:
    void iterationChanged(int iteration);
    
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    
private:
    void drawGrid();
    void drawAxes();
    void drawPointCloud(const PointCloud* cloud, const QColor& color);
    void drawTransformedSourceCloud();
    void updateCamera();
    
    // 点云数据
    PointCloud* m_sourceCloud;
    PointCloud* m_targetCloud;
    PointCloud* m_originalSource;  // 保存原始源点云用于迭代回放
    
    // 迭代历史
    std::vector<IterationResult> m_iterationHistory;
    int m_currentIteration;
    
    // 相机参数
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QVector3D m_cameraPos;
    QVector3D m_cameraTarget;
    QVector3D m_cameraUp;
    
    // 交互控制
    QPoint m_lastMousePos;
    bool m_isRotating;
    bool m_isPanning;
    float m_rotationX;
    float m_rotationY;
    float m_zoom;
    QVector3D m_panOffset;
    
    // 显示设置
    bool m_showGrid;
    bool m_showAxes;
    bool m_smoothRendering;
    QColor m_sourceColor;
    QColor m_targetColor;
    float m_pointSize;
    
    // 场景边界
    QVector3D m_sceneCenter;
    float m_sceneRadius;
};

#endif // POINTCLOUDVIEWER_H
