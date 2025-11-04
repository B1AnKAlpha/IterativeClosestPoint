#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include "Eigen/Eigen"

using namespace std;
using namespace Eigen;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 点结构
struct Point3D {
    double x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

// 读取LAS文件(简化版)
bool readLASFile(const string& filename, vector<Point3D>& points) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "无法打开文件: " << filename << endl;
        return false;
    }
    
    // 读取文件头
    char header[227];
    file.read(header, 227);
    
    // 获取点数据偏移和点数
    unsigned int offset_to_points = *((unsigned int*)(header + 96));
    unsigned int num_points = *((unsigned int*)(header + 107));
    unsigned short point_record_length = *((unsigned short*)(header + 105));
    
    // 获取比例因子和偏移量
    double x_scale = *((double*)(header + 131));
    double y_scale = *((double*)(header + 139));
    double z_scale = *((double*)(header + 147));
    double x_offset = *((double*)(header + 155));
    double y_offset = *((double*)(header + 163));
    double z_offset = *((double*)(header + 171));
    
    cout << "文件: " << filename << endl;
    cout << "  点数: " << num_points << endl;
    cout << "  比例: (" << x_scale << ", " << y_scale << ", " << z_scale << ")" << endl;
    cout << "  偏移: (" << x_offset << ", " << y_offset << ", " << z_offset << ")" << endl;
    
    // 定位到点数据
    file.seekg(offset_to_points, ios::beg);
    
    // 批量读取
    const int BATCH_SIZE = 10000;
    vector<char> buffer(BATCH_SIZE * point_record_length);
    points.reserve(num_points);
    
    for (unsigned int batch_start = 0; batch_start < num_points; batch_start += BATCH_SIZE) {
        int points_to_read = min(BATCH_SIZE, (int)(num_points - batch_start));
        int bytes_to_read = points_to_read * point_record_length;
        
        file.read(buffer.data(), bytes_to_read);
        
        for (int i = 0; i < points_to_read; i++) {
            int offset = i * point_record_length;
            int x_raw = *((int*)(buffer.data() + offset));
            int y_raw = *((int*)(buffer.data() + offset + 4));
            int z_raw = *((int*)(buffer.data() + offset + 8));
            
            Point3D p;
            p.x = x_raw * x_scale + x_offset;
            p.y = y_raw * y_scale + y_offset;
            p.z = z_raw * z_scale + z_offset;
            points.push_back(p);
        }
    }
    
    file.close();
    cout << "  成功读取 " << points.size() << " 个点" << endl;
    return true;
}

// 保存LAS文件
bool saveLASFile(const string& filename, const vector<Point3D>& points,
                 double x_scale, double y_scale, double z_scale,
                 double x_offset, double y_offset, double z_offset) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "无法创建文件: " << filename << endl;
        return false;
    }
    
    // 创建文件头
    char header[227] = {0};
    header[0] = 'L'; header[1] = 'A'; header[2] = 'S'; header[3] = 'F';
    header[24] = 1; header[25] = 2;
    
    *((unsigned short*)(header + 94)) = 227;
    *((unsigned int*)(header + 96)) = 227;
    *((unsigned int*)(header + 100)) = 0;
    header[104] = 0;
    *((unsigned short*)(header + 105)) = 20;
    *((unsigned int*)(header + 107)) = points.size();
    
    // 计算边界
    double x_min = points[0].x, x_max = points[0].x;
    double y_min = points[0].y, y_max = points[0].y;
    double z_min = points[0].z, z_max = points[0].z;
    
    for (const auto& p : points) {
        if (p.x < x_min) x_min = p.x;
        if (p.x > x_max) x_max = p.x;
        if (p.y < y_min) y_min = p.y;
        if (p.y > y_max) y_max = p.y;
        if (p.z < z_min) z_min = p.z;
        if (p.z > z_max) z_max = p.z;
    }
    
    *((double*)(header + 131)) = x_scale;
    *((double*)(header + 139)) = y_scale;
    *((double*)(header + 147)) = z_scale;
    *((double*)(header + 155)) = x_offset;
    *((double*)(header + 163)) = y_offset;
    *((double*)(header + 171)) = z_offset;
    *((double*)(header + 179)) = x_max;
    *((double*)(header + 187)) = x_min;
    *((double*)(header + 195)) = y_max;
    *((double*)(header + 203)) = y_min;
    *((double*)(header + 211)) = z_max;
    *((double*)(header + 219)) = z_min;
    
    file.write(header, 227);
    
    // 写入点数据
    for (const auto& p : points) {
        int x = (int)((p.x - x_offset) / x_scale);
        int y = (int)((p.y - y_offset) / y_scale);
        int z = (int)((p.z - z_offset) / z_scale);
        
        file.write((char*)&x, sizeof(int));
        file.write((char*)&y, sizeof(int));
        file.write((char*)&z, sizeof(int));
        
        unsigned short intensity = 0;
        unsigned char flags = 0, classification = 0, user_data = 0;
        char scan_angle = 0;
        unsigned short point_source_id = 0;
        
        file.write((char*)&intensity, sizeof(unsigned short));
        file.write((char*)&flags, sizeof(unsigned char));
        file.write((char*)&classification, sizeof(unsigned char));
        file.write((char*)&scan_angle, sizeof(char));
        file.write((char*)&user_data, sizeof(unsigned char));
        file.write((char*)&point_source_id, sizeof(unsigned short));
    }
    
    file.close();
    return true;
}

// 生成随机旋转矩阵(小角度)
Matrix3d randomRotation(double max_angle_deg) {
    double angle = (rand() / (double)RAND_MAX) * max_angle_deg * M_PI / 180.0;
    
    // 绕Z轴旋转(yaw)
    double yaw = angle;
    // 绕Y轴旋转(pitch)
    double pitch = (rand() / (double)RAND_MAX - 0.5) * angle;
    // 绕X轴旋转(roll)
    double roll = (rand() / (double)RAND_MAX - 0.5) * angle;
    
    Matrix3d Rz, Ry, Rx;
    Rz << cos(yaw), -sin(yaw), 0,
          sin(yaw),  cos(yaw), 0,
          0,         0,        1;
    
    Ry << cos(pitch),  0, sin(pitch),
          0,           1, 0,
          -sin(pitch), 0, cos(pitch);
    
    Rx << 1, 0,         0,
          0, cos(roll), -sin(roll),
          0, sin(roll),  cos(roll);
    
    return Rz * Ry * Rx;
}

int main() {
    system("chcp 65001>nul");
    srand(time(NULL));
    
    cout << "=== ICP配准测试程序 ===" << endl;
    cout << "\n步骤1: 读取sampled_source.las" << endl;
    
    vector<Point3D> points;
    if (!readLASFile("sampled_source.las", points)) {
        cout << "读取失败!" << endl;
        return -1;
    }
    
    // 保存原始比例和偏移
    double x_scale = 0.001, y_scale = 0.001, z_scale = 0.001;
    double x_offset = -139.219, y_offset = -137.327, z_offset = 0;
    
    cout << "\n步骤2: 应用随机变换" << endl;
    
    // 生成随机变换
    Matrix3d R_true = randomRotation(10.0);  // 最大10度旋转
    Vector3d t_true;
    t_true << (rand() / (double)RAND_MAX - 0.5) * 5.0,  // ±2.5米平移
              (rand() / (double)RAND_MAX - 0.5) * 5.0,
              (rand() / (double)RAND_MAX - 0.5) * 2.0;  // Z轴较小平移
    
    cout << "  真实旋转矩阵 R:" << endl;
    cout << R_true << endl;
    cout << "  真实平移向量 t: [" << t_true.transpose() << "]" << endl;
    
    // 应用变换到所有点
    vector<Point3D> transformed_points = points;
    for (auto& p : transformed_points) {
        Vector3d pt(p.x, p.y, p.z);
        Vector3d pt_new = R_true * pt + t_true;
        p.x = pt_new(0);
        p.y = pt_new(1);
        p.z = pt_new(2);
    }
    
    cout << "\n步骤3: 保存变换后的点云为 test_source_transformed.las" << endl;
    if (!saveLASFile("test_source_transformed.las", transformed_points,
                     x_scale, y_scale, z_scale, x_offset, y_offset, z_offset)) {
        cout << "保存失败!" << endl;
        return -1;
    }
    
    cout << "\n步骤4: 计算点云统计" << endl;
    
    // 原始点云范围
    double orig_x_min = points[0].x, orig_x_max = points[0].x;
    double orig_y_min = points[0].y, orig_y_max = points[0].y;
    double orig_z_min = points[0].z, orig_z_max = points[0].z;
    
    for (const auto& p : points) {
        if (p.x < orig_x_min) orig_x_min = p.x;
        if (p.x > orig_x_max) orig_x_max = p.x;
        if (p.y < orig_y_min) orig_y_min = p.y;
        if (p.y > orig_y_max) orig_y_max = p.y;
        if (p.z < orig_z_min) orig_z_min = p.z;
        if (p.z > orig_z_max) orig_z_max = p.z;
    }
    
    // 变换后点云范围
    double trans_x_min = transformed_points[0].x, trans_x_max = transformed_points[0].x;
    double trans_y_min = transformed_points[0].y, trans_y_max = transformed_points[0].y;
    double trans_z_min = transformed_points[0].z, trans_z_max = transformed_points[0].z;
    
    for (const auto& p : transformed_points) {
        if (p.x < trans_x_min) trans_x_min = p.x;
        if (p.x > trans_x_max) trans_x_max = p.x;
        if (p.y < trans_y_min) trans_y_min = p.y;
        if (p.y > trans_y_max) trans_y_max = p.y;
        if (p.z < trans_z_min) trans_z_min = p.z;
        if (p.z > trans_z_max) trans_z_max = p.z;
    }
    
    cout << "  原始点云范围:" << endl;
    cout << "    X: [" << orig_x_min << ", " << orig_x_max << "]" << endl;
    cout << "    Y: [" << orig_y_min << ", " << orig_y_max << "]" << endl;
    cout << "    Z: [" << orig_z_min << ", " << orig_z_max << "]" << endl;
    
    cout << "  变换后点云范围:" << endl;
    cout << "    X: [" << trans_x_min << ", " << trans_x_max << "]" << endl;
    cout << "    Y: [" << trans_y_min << ", " << trans_y_max << "]" << endl;
    cout << "    Z: [" << trans_z_min << ", " << trans_z_max << "]" << endl;
    
    cout << "\n===========================================" << endl;
    cout << "测试数据准备完成!" << endl;
    cout << "===========================================" << endl;
    cout << "\n现在请运行: icp_registration.exe" << endl;
    cout << "使用以下文件进行配准:" << endl;
    cout << "  源点云: test_source_transformed.las (变换后)" << endl;
    cout << "  目标点云: sampled_source.las (原始)" << endl;
    cout << "\n如果ICP成功,应该能恢复出接近上述的真实变换矩阵。" << endl;
    
    cout << "\n请按回车键退出..." << endl;
    cin.get();
    
    return 0;
}
