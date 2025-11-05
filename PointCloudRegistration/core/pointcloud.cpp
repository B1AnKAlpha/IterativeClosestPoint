#include "pointcloud.h"
#include <algorithm>
#include <cmath>
#include <limits>

PointCloud::PointCloud()
    : color(Qt::white)
    , pointSize(2.0f)
    , minX(0), maxX(0), minY(0), maxY(0), minZ(0), maxZ(0)
    , m_boundsComputed(false)
{
}

PointCloud::~PointCloud()
{
}

void PointCloud::clear()
{
    points.clear();
    m_boundsComputed = false;
}

void PointCloud::computeBounds()
{
    if (points.empty()) {
        minX = maxX = minY = maxY = minZ = maxZ = 0;
        m_boundsComputed = false;
        return;
    }
    
    minX = minY = minZ = std::numeric_limits<double>::max();
    maxX = maxY = maxZ = std::numeric_limits<double>::lowest();
    
    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        minZ = std::min(minZ, p.z);
        maxZ = std::max(maxZ, p.z);
    }
    
    m_boundsComputed = true;
}

QVector3D PointCloud::getCenter() const
{
    if (!m_boundsComputed || points.empty()) {
        return QVector3D(0, 0, 0);
    }
    
    return QVector3D(
        static_cast<float>((minX + maxX) / 2.0),
        static_cast<float>((minY + maxY) / 2.0),
        static_cast<float>((minZ + maxZ) / 2.0)
    );
}

double PointCloud::getRadius() const
{
    if (!m_boundsComputed) {
        return 0.0;
    }
    
    double dx = maxX - minX;
    double dy = maxY - minY;
    double dz = maxZ - minZ;
    
    return std::sqrt(dx*dx + dy*dy + dz*dz) / 2.0;
}

void PointCloud::applyTransform(const double R[3][3], const double t[3])
{
    for (auto& p : points) {
        double nx = R[0][0] * p.x + R[0][1] * p.y + R[0][2] * p.z + t[0];
        double ny = R[1][0] * p.x + R[1][1] * p.y + R[1][2] * p.z + t[1];
        double nz = R[2][0] * p.x + R[2][1] * p.y + R[2][2] * p.z + t[2];
        
        p.x = nx;
        p.y = ny;
        p.z = nz;
    }
    
    m_boundsComputed = false;
}

void PointCloud::applyTransformMatrix(const std::vector<std::vector<double>>& transform)
{
    if (transform.size() != 4 || transform[0].size() != 4) {
        return;
    }
    
    for (auto& p : points) {
        double nx = transform[0][0] * p.x + transform[0][1] * p.y + transform[0][2] * p.z + transform[0][3];
        double ny = transform[1][0] * p.x + transform[1][1] * p.y + transform[1][2] * p.z + transform[1][3];
        double nz = transform[2][0] * p.x + transform[2][1] * p.y + transform[2][2] * p.z + transform[2][3];
        
        p.x = nx;
        p.y = ny;
        p.z = nz;
    }
    
    m_boundsComputed = false;
}

PointCloud* PointCloud::downsample(int targetSize) const
{
    if (points.empty() || targetSize <= 0) {
        return nullptr;
    }
    
    PointCloud* sampled = new PointCloud();
    sampled->color = this->color;
    sampled->pointSize = this->pointSize;
    
    if (static_cast<int>(points.size()) <= targetSize) {
        sampled->points = this->points;
    } else {
        double step = static_cast<double>(points.size()) / targetSize;
        for (int i = 0; i < targetSize; ++i) {
            int idx = static_cast<int>(i * step);
            sampled->points.push_back(points[idx]);
        }
    }
    
    return sampled;
}
