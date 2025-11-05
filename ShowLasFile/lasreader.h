#ifndef LASREADER_H
#define LASREADER_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

// 点结构
struct Point3D {
    float x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// 点云结构
class PointCloud {
public:
    std::vector<Point3D> points;
    
    // 存储原始LAS文件的偏移量和比例因子
    double x_scale, y_scale, z_scale;
    double x_offset, y_offset, z_offset;
    
    // 边界信息
    double x_min, x_max, y_min, y_max, z_min, z_max;
    
    PointCloud() : x_scale(0.001), y_scale(0.001), z_scale(0.001),
                   x_offset(0), y_offset(0), z_offset(0),
                   x_min(0), x_max(0), y_min(0), y_max(0), z_min(0), z_max(0) {}
    
    void addPoint(const Point3D& p) {
        points.push_back(p);
    }
    
    void reserve(size_t capacity) {
        points.reserve(capacity);
    }
    
    size_t size() const {
        return points.size();
    }
    
    void calculateBounds() {
        if (points.empty()) return;
        
        x_min = x_max = points[0].x;
        y_min = y_max = points[0].y;
        z_min = z_max = points[0].z;
        
        for (const auto& p : points) {
            if (p.x < x_min) x_min = p.x;
            if (p.x > x_max) x_max = p.x;
            if (p.y < y_min) y_min = p.y;
            if (p.y > y_max) y_max = p.y;
            if (p.z < z_min) z_min = p.z;
            if (p.z > z_max) z_max = p.z;
        }
    }
};

// 读取LAS文件
class LASReader {
public:
    static bool readLASFile(const std::string& filename, PointCloud& cloud, int sample_rate = 1);
};

#endif // LASREADER_H
