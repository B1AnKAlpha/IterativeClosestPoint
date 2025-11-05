#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include <vector>
#include <string>
#include <QVector3D>
#include <QColor>

/**
 * @brief 3D点结构
 */
struct Point3D {
    double x, y, z;
    
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    QVector3D toQVector3D() const {
        return QVector3D(static_cast<float>(x), 
                        static_cast<float>(y), 
                        static_cast<float>(z));
    }
};

/**
 * @brief 点云类
 * 
 * 包含点云数据、边界信息和操作方法
 */
class PointCloud
{
public:
    PointCloud();
    ~PointCloud();
    
    // 基础操作
    void clear();
    size_t size() const { return points.size(); }
    bool empty() const { return points.empty(); }
    
    // 点云数据
    std::vector<Point3D> points;
    
    // 显示属性
    QColor color;
    float pointSize;
    
    // 边界信息
    void computeBounds();
    double minX, maxX, minY, maxY, minZ, maxZ;
    
    // 中心和缩放
    QVector3D getCenter() const;
    double getRadius() const;
    
    // 变换操作
    void applyTransform(const double R[3][3], const double t[3]);
    void applyTransformMatrix(const std::vector<std::vector<double>>& transform);
    
    // 采样
    PointCloud* downsample(int targetSize) const;
    
private:
    bool m_boundsComputed;
};

#endif // POINTCLOUD_H
