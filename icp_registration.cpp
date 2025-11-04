#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <numeric>
#include "Eigen/Eigen"

using namespace std;
using namespace Eigen;

// 点结构
struct Point3D {
    double x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

// 八叉树节点
class OctreeNode {
public:
    double min_x, max_x, min_y, max_y, min_z, max_z;  // 边界
    vector<int> point_indices;  // 存储点的索引
    OctreeNode* children[8];  // 8个子节点
    bool is_leaf;
    
    OctreeNode(double minx, double maxx, double miny, double maxy, double minz, double maxz)
        : min_x(minx), max_x(maxx), min_y(miny), max_y(maxy), min_z(minz), max_z(maxz), 
          is_leaf(true) {
        for (int i = 0; i < 8; i++) children[i] = nullptr;
    }
    
    ~OctreeNode() {
        for (int i = 0; i < 8; i++) {
            if (children[i]) delete children[i];
        }
    }
    
    // 判断点是否在节点范围内
    bool contains(const Point3D& p) const {
        return p.x >= min_x && p.x <= max_x &&
               p.y >= min_y && p.y <= max_y &&
               p.z >= min_z && p.z <= max_z;
    }
    
    // 计算点到节点的最小距离
    double minDistanceTo(const Point3D& p) const {
        double dx = max(0.0, max(min_x - p.x, p.x - max_x));
        double dy = max(0.0, max(min_y - p.y, p.y - max_y));
        double dz = max(0.0, max(min_z - p.z, p.z - max_z));
        return sqrt(dx*dx + dy*dy + dz*dz);
    }
};

// 八叉树类
class Octree {
private:
    OctreeNode* root;
    const vector<Point3D>* points;  // 指向点云数据
    int max_points_per_node;  // 每个叶节点最多存储的点数
    int max_depth;  // 最大深度
    
    void buildTree(OctreeNode* node, const vector<int>& indices, int depth) {
        if (indices.size() <= max_points_per_node || depth >= max_depth) {
            node->point_indices = indices;
            node->is_leaf = true;
            return;
        }
        
        node->is_leaf = false;
        
        // 计算中心点
        double mid_x = (node->min_x + node->max_x) / 2;
        double mid_y = (node->min_y + node->max_y) / 2;
        double mid_z = (node->min_z + node->max_z) / 2;
        
        // 为8个子节点分配点
        vector<vector<int>> child_indices(8);
        for (int idx : indices) {
            const Point3D& p = (*points)[idx];
            int octant = 0;
            if (p.x > mid_x) octant |= 1;
            if (p.y > mid_y) octant |= 2;
            if (p.z > mid_z) octant |= 4;
            child_indices[octant].push_back(idx);
        }
        
        // 递归创建子节点
        for (int i = 0; i < 8; i++) {
            if (!child_indices[i].empty()) {
                double minx = (i & 1) ? mid_x : node->min_x;
                double maxx = (i & 1) ? node->max_x : mid_x;
                double miny = (i & 2) ? mid_y : node->min_y;
                double maxy = (i & 2) ? node->max_y : mid_y;
                double minz = (i & 4) ? mid_z : node->min_z;
                double maxz = (i & 4) ? node->max_z : mid_z;
                
                node->children[i] = new OctreeNode(minx, maxx, miny, maxy, minz, maxz);
                buildTree(node->children[i], child_indices[i], depth + 1);
            }
        }
    }
    
    void searchNearest(OctreeNode* node, const Point3D& query, 
                      int& best_idx, double& best_dist_sq) const {
        if (!node) return;
        
        // 如果节点的最小距离大于当前最佳距离,剪枝
        double min_dist = node->minDistanceTo(query);
        if (min_dist * min_dist >= best_dist_sq) return;
        
        if (node->is_leaf) {
            // 叶节点:检查所有点
            for (int idx : node->point_indices) {
                const Point3D& p = (*points)[idx];
                double dx = p.x - query.x;
                double dy = p.y - query.y;
                double dz = p.z - query.z;
                double dist_sq = dx*dx + dy*dy + dz*dz;
                
                if (dist_sq < best_dist_sq) {
                    best_dist_sq = dist_sq;
                    best_idx = idx;
                }
            }
        } else {
            // 内部节点:按距离排序子节点,优先搜索近的
            struct ChildDist {
                int index;
                double dist;
            };
            vector<ChildDist> child_dists;
            
            for (int i = 0; i < 8; i++) {
                if (node->children[i]) {
                    double dist = node->children[i]->minDistanceTo(query);
                    child_dists.push_back({i, dist});
                }
            }
            
            sort(child_dists.begin(), child_dists.end(), 
                 [](const ChildDist& a, const ChildDist& b) { return a.dist < b.dist; });
            
            for (const auto& cd : child_dists) {
                searchNearest(node->children[cd.index], query, best_idx, best_dist_sq);
            }
        }
    }
    
public:
    Octree(const vector<Point3D>& pts, int max_pts = 10, int max_d = 20) 
        : points(&pts), max_points_per_node(max_pts), max_depth(max_d), root(nullptr) {
        
        if (pts.empty()) return;
        
        // 计算边界
        double min_x = pts[0].x, max_x = pts[0].x;
        double min_y = pts[0].y, max_y = pts[0].y;
        double min_z = pts[0].z, max_z = pts[0].z;
        
        for (const auto& p : pts) {
            if (p.x < min_x) min_x = p.x;
            if (p.x > max_x) max_x = p.x;
            if (p.y < min_y) min_y = p.y;
            if (p.y > max_y) max_y = p.y;
            if (p.z < min_z) min_z = p.z;
            if (p.z > max_z) max_z = p.z;
        }
        
        // 稍微扩大边界以包含边界点
        double eps = 0.001;
        min_x -= eps; max_x += eps;
        min_y -= eps; max_y += eps;
        min_z -= eps; max_z += eps;
        
        // 创建根节点
        root = new OctreeNode(min_x, max_x, min_y, max_y, min_z, max_z);
        
        // 建立索引列表
        vector<int> all_indices(pts.size());
        for (size_t i = 0; i < pts.size(); i++) {
            all_indices[i] = i;
        }
        
        // 构建树
        buildTree(root, all_indices, 0);
    }
    
    ~Octree() {
        if (root) delete root;
    }
    
    // 查找最近点
    int findNearest(const Point3D& query) const {
        if (!root || points->empty()) return 0;
        
        int best_idx = 0;
        double best_dist_sq = 1e20;
        
        searchNearest(root, query, best_idx, best_dist_sq);
        return best_idx;
    }
};

// 点云结构
class PointCloud {
public:
    vector<Point3D> points;
    
    // 存储原始LAS文件的偏移量和比例因子
    double x_scale, y_scale, z_scale;
    double x_offset, y_offset, z_offset;
    
    PointCloud() : x_scale(0.001), y_scale(0.001), z_scale(0.001),
                   x_offset(0), y_offset(0), z_offset(0) {}
    
    void addPoint(const Point3D& p) {
        points.push_back(p);
    }
    
    // 预分配内存
    void reserve(size_t capacity) {
        points.reserve(capacity);
    }
    
    size_t size() const {
        return points.size();
    }
    
    Point3D getCentroid() const {
        Point3D centroid;
        for (const auto& p : points) {
            centroid.x += p.x;
            centroid.y += p.y;
            centroid.z += p.z;
        }
        centroid.x /= points.size();
        centroid.y /= points.size();
        centroid.z /= points.size();
        return centroid;
    }
};

// 读取LAS文件(简化版本 - 读取XYZ坐标)
bool readLASFile(const string& filename, PointCloud& cloud) {
    cout << "  正在尝试打开文件: " << filename << endl;
    
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "  错误: 无法打开文件: " << filename << endl;
        return false;
    }
    
    // 设置更大的I/O缓冲区(1MB)以提高读取速度
    const int BUFFER_SIZE = 1024 * 1024;
    vector<char> io_buffer(BUFFER_SIZE);  // 使用vector自动管理内存
    file.rdbuf()->pubsetbuf(io_buffer.data(), BUFFER_SIZE);
    
    cout << "  文件打开成功" << endl;
    
    // 读取LAS文件头
    char header[227];
    file.read(header, 227);
    
    if (!file) {
        cerr << "  错误: 无法读取文件头" << endl;
        file.close();
        return false;
    }
    
    cout << "  文件签名: ";
    for (int i = 0; i < 4; i++) {
        cout << header[i];
    }
    cout << endl;
    
    // 获取点数据偏移
    unsigned int offset_to_points = *((unsigned int*)(header + 96));
    cout << "  点数据偏移: " << offset_to_points << endl;
    
    // 获取点数
    unsigned int num_points = *((unsigned int*)(header + 107));
    cout << "  点数量: " << num_points << endl;
    
    // 获取点数据格式和记录长度
    unsigned short point_record_length = *((unsigned short*)(header + 105));
    
    if (num_points == 0 || num_points > 100000000) {
        cerr << "  警告: 点数量异常: " << num_points << endl;
        file.close();
        return false;
    }
    
    // 获取比例因子和偏移量
    double x_scale = *((double*)(header + 131));
    double y_scale = *((double*)(header + 139));
    double z_scale = *((double*)(header + 147));
    double x_offset = *((double*)(header + 155));
    double y_offset = *((double*)(header + 163));
    double z_offset = *((double*)(header + 171));
    
    cout << "  比例因子: x=" << x_scale << ", y=" << y_scale << ", z=" << z_scale << endl;
    cout << "  偏移量: x=" << x_offset << ", y=" << y_offset << ", z=" << z_offset << endl;
    
    // 保存原始的比例因子和偏移量到点云对象
    cloud.x_scale = x_scale;
    cloud.y_scale = y_scale;
    cloud.z_scale = z_scale;
    cloud.x_offset = x_offset;
    cloud.y_offset = y_offset;
    cloud.z_offset = z_offset;
    
    // 预分配内存空间
    cloud.reserve(num_points);
    
    // 定位到点数据
    file.seekg(offset_to_points, ios::beg);
    
    if (!file) {
        cerr << "  错误: 无法定位到点数据" << endl;
        file.close();
        return false;
    }
    
    cout << "  开始读取点数据..." << endl;
    
    // 批量读取点数据
    const int BATCH_SIZE = 10000;  // 每次读取10000个点
    vector<char> buffer(BATCH_SIZE * point_record_length);
    
    int read_count = 0;
    int last_progress = 0;
    
    for (unsigned int batch_start = 0; batch_start < num_points; batch_start += BATCH_SIZE) {
        int points_to_read = min(BATCH_SIZE, (int)(num_points - batch_start));
        int bytes_to_read = points_to_read * point_record_length;
        
        // 一次性读取一批点
        file.read(buffer.data(), bytes_to_read);
        
        if (!file && !file.eof()) {
            cerr << "  警告: 在读取批次时失败,已读取 " << read_count << " 个点" << endl;
            break;
        }
        
        // 解析批次中的点
        for (int i = 0; i < points_to_read; i++) {
            int offset = i * point_record_length;
            int x_raw = *((int*)(buffer.data() + offset));
            int y_raw = *((int*)(buffer.data() + offset + 4));
            int z_raw = *((int*)(buffer.data() + offset + 8));
            
            Point3D point;
            point.x = x_raw * x_scale + x_offset;
            point.y = y_raw * y_scale + y_offset;
            point.z = z_raw * z_scale + z_offset;
            
            cloud.points.push_back(point);
            read_count++;
        }
        
        // 显示进度(每50000个点显示一次)
        int progress = (read_count / 50000) * 50000;
        if (progress > last_progress && progress > 0) {
            cout << "  已读取 " << read_count << " / " << num_points << " 个点 (" 
                 << (read_count * 100 / num_points) << "%)" << endl;
            last_progress = progress;
        }
    }
    
    file.close();
    cout << "  成功读取 " << read_count << " 个点" << endl;
    
    return read_count > 0;
}

// 计算两点之间的欧氏距离
double distance(const Point3D& p1, const Point3D& p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    double dz = p1.z - p2.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// 使用SVD计算最佳拟合变换矩阵 (标准ICP方法)
Eigen::Matrix4d best_fit_transform(const Eigen::MatrixXd &A, const Eigen::MatrixXd &B) {
    // A: 源点云 (N x 3)
    // B: 目标点云对应点 (N x 3)
    
    Eigen::Matrix4d T = Eigen::MatrixXd::Identity(4, 4);
    Eigen::Vector3d centroid_A(0, 0, 0);
    Eigen::Vector3d centroid_B(0, 0, 0);
    int row = A.rows();
    
    // 计算质心
    for (int i = 0; i < row; i++) {
        centroid_A += A.block<1, 3>(i, 0).transpose();
        centroid_B += B.block<1, 3>(i, 0).transpose();
    }
    centroid_A /= row;
    centroid_B /= row;
    
    // 去质心
    Eigen::MatrixXd AA = A;
    Eigen::MatrixXd BB = B;
    for (int i = 0; i < row; i++) {
        AA.block<1, 3>(i, 0) = A.block<1, 3>(i, 0) - centroid_A.transpose();
        BB.block<1, 3>(i, 0) = B.block<1, 3>(i, 0) - centroid_B.transpose();
    }
    
    // 计算协方差矩阵 H = AA^T * BB
    Eigen::MatrixXd H = AA.transpose() * BB;
    
    // SVD分解
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::MatrixXd U = svd.matrixU();
    Eigen::MatrixXd V = svd.matrixV();
    Eigen::MatrixXd Vt = V.transpose();
    
    // 计算旋转矩阵 R = V * U^T
    Eigen::Matrix3d R = Vt.transpose() * U.transpose();
    
    // 处理反射的情况
    if (R.determinant() < 0) {
        Vt.block<1, 3>(2, 0) *= -1;
        R = Vt.transpose() * U.transpose();
    }
    
    // 计算平移向量 t = centroid_B - R * centroid_A
    Eigen::Vector3d t = centroid_B - R * centroid_A;
    
    // 组装4x4变换矩阵
    T.block<3, 3>(0, 0) = R;
    T.block<3, 1>(0, 3) = t;
    
    return T;
}

// ICP算法主函数(使用八叉树优化 + SVD标准实现)
void ICP(PointCloud& source, const PointCloud& target,
         int max_iterations, double tolerance,
         double final_R[3][3], double final_t[3],
         vector<Eigen::Matrix4d>* iteration_transforms = nullptr) {
    
    cout << "\n开始ICP精匹配..." << endl;
    cout << "源点云: " << source.size() << " 个点" << endl;
    cout << "目标点云: " << target.size() << " 个点" << endl;
    
    // 构建目标点云的八叉树
    cout << "构建八叉树索引..." << flush;
    Octree octree(target.points, 10, 20);
    cout << " 完成!" << endl;
    
    int row = source.size();
    
    // 转换为Eigen矩阵格式
    Eigen::MatrixXd src = Eigen::MatrixXd::Ones(4, row);  // 齐次坐标
    Eigen::MatrixXd src3d = Eigen::MatrixXd::Ones(3, row);
    
    for (int i = 0; i < row; i++) {
        src3d(0, i) = source.points[i].x;
        src3d(1, i) = source.points[i].y;
        src3d(2, i) = source.points[i].z;
        src(0, i) = source.points[i].x;
        src(1, i) = source.points[i].y;
        src(2, i) = source.points[i].z;
    }
    
    Eigen::Matrix4d T = Eigen::MatrixXd::Identity(4, 4);
    Eigen::Matrix4d T_cumulative = Eigen::MatrixXd::Identity(4, 4);  // 累积变换
    double prev_error = 1e10;
    int no_improvement_count = 0;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        cout << "迭代 " << iter + 1 << "/" << max_iterations << " ..." << flush;
        
        // 步骤1: 使用八叉树找到最近点对应
        vector<int> correspondences(row);
        Eigen::MatrixXd dst_matched = Eigen::MatrixXd::Ones(3, row);
        
        for (int i = 0; i < row; i++) {
            Point3D query;
            query.x = src3d(0, i);
            query.y = src3d(1, i);
            query.z = src3d(2, i);
            
            int nearest_idx = octree.findNearest(query);
            correspondences[i] = nearest_idx;
            
            dst_matched(0, i) = target.points[nearest_idx].x;
            dst_matched(1, i) = target.points[nearest_idx].y;
            dst_matched(2, i) = target.points[nearest_idx].z;
        }
        
        // 步骤2: 计算所有点对的距离
        vector<double> distances(row);
        for (int i = 0; i < row; i++) {
            Point3D p_src;
            p_src.x = src3d(0, i);
            p_src.y = src3d(1, i);
            p_src.z = src3d(2, i);
            
            distances[i] = distance(p_src, target.points[correspondences[i]]);
        }
        
        // 计算均值和标准差
        double mean_dist = 0;
        for (double d : distances) {
            mean_dist += d;
        }
        mean_dist /= row;
        
        double variance = 0;
        for (double d : distances) {
            variance += (d - mean_dist) * (d - mean_dist);
        }
        double std_dev = sqrt(variance / row);
        
        // 使用3-sigma原则设置距离阈值剔除离群点
        double threshold = mean_dist + 3.0 * std_dev;
        
        // 统计有效点对（非离群点）
        vector<int> valid_indices;
        for (int i = 0; i < row; i++) {
            if (distances[i] <= threshold) {
                valid_indices.push_back(i);
            }
        }
        
        int valid_count = valid_indices.size();
        int outlier_count = row - valid_count;
        
        // 只用有效点计算RMSE
        double sum_sq = 0;
        for (int idx : valid_indices) {
            sum_sq += distances[idx] * distances[idx];
        }
        double mean_error = (valid_count > 0) ? sqrt(sum_sq / valid_count) : 0;
        
        cout << " RMSE = " << mean_error 
             << " (有效点: " << valid_count << "/" << row 
             << ", 剔除离群点: " << outlier_count << ")" << endl;
        
        // 步骤3: 检查收敛
        double improvement = prev_error - mean_error;
        if (abs(improvement) < tolerance) {
            no_improvement_count++;
            if (no_improvement_count >= 3) {
                cout << "收敛达到! 迭代次数: " << iter + 1 << endl;
                break;
            }
        } else {
            no_improvement_count = 0;
        }
        
        if (mean_error > prev_error * 1.1) {
            cout << "警告: 误差增加,停止迭代。" << endl;
            break;
        }
        
        prev_error = mean_error;
        
        // 步骤4: 使用SVD计算最佳变换（只使用非离群点）
        if (valid_count < 3) {
            cout << "警告: 有效点对太少（< 3），无法计算变换，停止迭代。" << endl;
            break;
        }
        
        // 构建只包含有效点的矩阵
        Eigen::MatrixXd src_valid = Eigen::MatrixXd::Ones(3, valid_count);
        Eigen::MatrixXd dst_valid = Eigen::MatrixXd::Ones(3, valid_count);
        
        for (int i = 0; i < valid_count; i++) {
            int idx = valid_indices[i];
            src_valid(0, i) = src3d(0, idx);
            src_valid(1, i) = src3d(1, idx);
            src_valid(2, i) = src3d(2, idx);
            
            dst_valid(0, i) = dst_matched(0, idx);
            dst_valid(1, i) = dst_matched(1, idx);
            dst_valid(2, i) = dst_matched(2, idx);
        }
        
        T = best_fit_transform(src_valid.transpose(), dst_valid.transpose());
        
        // 累积变换矩阵
        T_cumulative = T * T_cumulative;
        
        // 保存每次迭代的累积变换
        if (iteration_transforms != nullptr) {
            iteration_transforms->push_back(T_cumulative);
        }
        
        // 步骤5: 应用变换到源点云
        src = T * src;
        for (int i = 0; i < row; i++) {
            src3d(0, i) = src(0, i);
            src3d(1, i) = src(1, i);
            src3d(2, i) = src(2, i);
        }
    }
    
    cout << "最终RMSE: " << prev_error << endl;
    
    // 将结果写回源点云
    for (int i = 0; i < row; i++) {
        source.points[i].x = src3d(0, i);
        source.points[i].y = src3d(1, i);
        source.points[i].z = src3d(2, i);
    }
    
    // 提取最终的旋转矩阵和平移向量
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            final_R[i][j] = T(i, j);
        }
        final_t[i] = T(i, 3);
    }
}

// 保存变换参数到文件
void saveTransformation(double R[3][3], double t[3], const string& filename, 
                       const vector<Eigen::Matrix4d>* iteration_transforms = nullptr) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "无法创建变换参数文件: " << filename << endl;
        return;
    }
    
    file << "ICP配准变换参数" << endl;
    file << "==================" << endl << endl;
    
    file << "说明: 将源点云变换到目标点云坐标系下的变换矩阵" << endl;
    file << "变换公式: P_target = R * P_source + t" << endl << endl;
    
    // 保存每次迭代的变换参数
    if (iteration_transforms != nullptr && !iteration_transforms->empty()) {
        file << "==================" << endl;
        file << "迭代过程变换参数" << endl;
        file << "==================" << endl << endl;
        file.precision(10);
        
        for (size_t iter = 0; iter < iteration_transforms->size(); iter++) {
            const Eigen::Matrix4d& T = (*iteration_transforms)[iter];
            file << "--- 迭代 " << (iter + 1) << " ---" << endl;
            file << "旋转矩阵 R:" << endl;
            for (int i = 0; i < 3; i++) {
                file << "  [";
                for (int j = 0; j < 3; j++) {
                    file << T(i, j);
                    if (j < 2) file << ", ";
                }
                file << "]" << endl;
            }
            file << "平移向量 t:" << endl;
            file << "  [" << T(0, 3) << ", " << T(1, 3) << ", " << T(2, 3) << "]" << endl;
            file << endl;
        }
        file << endl;
    }
    
    file << "==================" << endl;
    file << "最终变换参数" << endl;
    file << "==================" << endl << endl;
    
    file << "旋转矩阵 R (3x3):" << endl;
    file.precision(10);
    for (int i = 0; i < 3; i++) {
        file << "  [";
        for (int j = 0; j < 3; j++) {
            file << R[i][j];
            if (j < 2) file << ", ";
        }
        file << "]" << endl;
    }
    
    file << endl << "平移向量 t (3x1):" << endl;
    file << "  [" << t[0] << ", " << t[1] << ", " << t[2] << "]" << endl;
    
    file << endl << "变换矩阵 (齐次坐标形式 4x4):" << endl;
    for (int i = 0; i < 3; i++) {
        file << "  [";
        for (int j = 0; j < 3; j++) {
            file << R[i][j] << ", ";
        }
        file << t[i] << "]" << endl;
    }
    file << "  [0, 0, 0, 1]" << endl;
    
    file.close();
    cout << "变换参数已保存到: " << filename << endl;
}

// 保存配准结果为LAS格式
void saveResultAsLAS(const PointCloud& cloud, const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "无法创建输出文件: " << filename << endl;
        return;
    }
    
    // 创建LAS文件头(227字节)
    char header[227] = {0};
    
    // 文件签名 "LASF"
    header[0] = 'L'; header[1] = 'A'; header[2] = 'S'; header[3] = 'F';
    
    // 文件源ID
    *((unsigned short*)(header + 4)) = 0;
    
    // 版本号: 1.2
    header[24] = 1;  // 主版本号
    header[25] = 2;  // 次版本号
    
    // 系统标识符
    string system_id = "ICP Registration";
    for (size_t i = 0; i < system_id.length() && i < 32; i++) {
        header[26 + i] = system_id[i];
    }
    
    // 生成软件
    string software = "Custom ICP";
    for (size_t i = 0; i < software.length() && i < 32; i++) {
        header[58 + i] = software[i];
    }
    
    // 文件创建日期
    *((unsigned short*)(header + 90)) = 307;  // 天数
    *((unsigned short*)(header + 92)) = 2025; // 年份
    
    // 文件头大小
    *((unsigned short*)(header + 94)) = 227;
    
    // 点数据偏移
    *((unsigned int*)(header + 96)) = 227;
    
    // 可变长度记录数量
    *((unsigned int*)(header + 100)) = 0;
    
    // 点数据格式(Format 0)
    header[104] = 0;
    
    // 点记录长度(Format 0 = 20字节)
    *((unsigned short*)(header + 105)) = 20;
    
    // 点数量
    *((unsigned int*)(header + 107)) = cloud.size();
    
    // 计算边界值
    double x_min = cloud.points[0].x, x_max = cloud.points[0].x;
    double y_min = cloud.points[0].y, y_max = cloud.points[0].y;
    double z_min = cloud.points[0].z, z_max = cloud.points[0].z;
    
    for (const auto& p : cloud.points) {
        if (p.x < x_min) x_min = p.x;
        if (p.x > x_max) x_max = p.x;
        if (p.y < y_min) y_min = p.y;
        if (p.y > y_max) y_max = p.y;
        if (p.z < z_min) z_min = p.z;
        if (p.z > z_max) z_max = p.z;
    }
    
    // 使用原始文件的比例因子和偏移量(关键修复!)
    *((double*)(header + 131)) = cloud.x_scale;   // x scale
    *((double*)(header + 139)) = cloud.y_scale;   // y scale
    *((double*)(header + 147)) = cloud.z_scale;   // z scale
    
    *((double*)(header + 155)) = cloud.x_offset;  // x offset
    *((double*)(header + 163)) = cloud.y_offset;  // y offset
    *((double*)(header + 171)) = cloud.z_offset;  // z offset
    
    // 边界值
    *((double*)(header + 179)) = x_max;  // x max
    *((double*)(header + 187)) = x_min;  // x min
    *((double*)(header + 195)) = y_max;  // y max
    *((double*)(header + 203)) = y_min;  // y min
    *((double*)(header + 211)) = z_max;  // z max
    *((double*)(header + 219)) = z_min;  // z min
    
    // 写入文件头
    file.write(header, 227);
    
    // 写入点数据
    for (const auto& p : cloud.points) {
        // 使用原始的偏移量和比例因子转换为整数坐标
        int x = (int)((p.x - cloud.x_offset) / cloud.x_scale);
        int y = (int)((p.y - cloud.y_offset) / cloud.y_scale);
        int z = (int)((p.z - cloud.z_offset) / cloud.z_scale);
        
        file.write((char*)&x, sizeof(int));
        file.write((char*)&y, sizeof(int));
        file.write((char*)&z, sizeof(int));
        
        // 写入其他属性(强度、返回号等,全部设为0)
        unsigned short intensity = 0;
        unsigned char flags = 0;
        unsigned char classification = 0;
        char scan_angle = 0;
        unsigned char user_data = 0;
        unsigned short point_source_id = 0;
        
        file.write((char*)&intensity, sizeof(unsigned short));
        file.write((char*)&flags, sizeof(unsigned char));
        file.write((char*)&classification, sizeof(unsigned char));
        file.write((char*)&scan_angle, sizeof(char));
        file.write((char*)&user_data, sizeof(unsigned char));
        file.write((char*)&point_source_id, sizeof(unsigned short));
    }
    
    file.close();
    cout << "结果已保存到: " << filename << endl;
}

int main() {
    system("chcp 65001>nul");
    cout << "=== ICP点云精匹配程序 ===" << endl;
    cout << "程序启动..." << endl;
    
    // 读取两个LAS文件
    PointCloud source_cloud, target_cloud;
    
    string source_file = "Scan_096_origin.las";
    string target_file = "Scannew_099.las";
    
    cout << "\n读取源点云: " << source_file << endl;
    if (!readLASFile(source_file, source_cloud)) {
        cout << "错误: 无法读取源点云文件!" << endl;
        cout << "按任意键退出..." << endl;
        cin.get();
        return -1;
    }
    
    cout << "\n读取目标点云: " << target_file << endl;
    if (!readLASFile(target_file, target_cloud)) {
        cout << "错误: 无法读取目标点云文件!" << endl;
        cout << "按任意键退出..." << endl;
        cin.get();
        return -1;
    }
    
    if (source_cloud.size() == 0 || target_cloud.size() == 0) {
        cout << "错误: 点云为空!" << endl;
        cout << "请按回车键退出..." << endl;
        cin.get();
        return -1;
    }
    
    // 下采样以加快处理速度
    // 采样率建议: 
    //   - 快速测试: 500-1000 (几千个点)
    //   - 中等精度: 100-200 (几万个点)
    //   - 高精度: 10-50 (几十万个点)
    PointCloud source_sampled, target_sampled;
    int sample_rate = 50;  // 推荐值,可根据需要调整
    
    cout << "\n下采样中 (采样率: 1/" << sample_rate << ")..." << endl;
    
    // 让两个采样点云使用相同的偏移量和比例因子
    source_sampled.x_scale = source_cloud.x_scale;
    source_sampled.y_scale = source_cloud.y_scale;
    source_sampled.z_scale = source_cloud.z_scale;
    source_sampled.x_offset = source_cloud.x_offset;
    source_sampled.y_offset = source_cloud.y_offset;
    source_sampled.z_offset = source_cloud.z_offset;
    
    // 目标点云也使用源点云的偏移量
    target_sampled.x_scale = source_cloud.x_scale;
    target_sampled.y_scale = source_cloud.y_scale;
    target_sampled.z_scale = source_cloud.z_scale;
    target_sampled.x_offset = source_cloud.x_offset;
    target_sampled.y_offset = source_cloud.y_offset;
    target_sampled.z_offset = source_cloud.z_offset;
    
    for (size_t i = 0; i < source_cloud.size(); i += sample_rate) {
        source_sampled.addPoint(source_cloud.points[i]);
    }
    for (size_t i = 0; i < target_cloud.size(); i += sample_rate) {
        target_sampled.addPoint(target_cloud.points[i]);
    }
    
    cout << "\n下采样后:" << endl;
    cout << "源点云: " << source_sampled.size() << " 个点" << endl;
    cout << "目标点云: " << target_sampled.size() << " 个点" << endl;
    
    // 保存下采样后的点云
    cout << "\n保存下采样后的点云..." << endl;
    saveResultAsLAS(source_sampled, "sampled_source.las");
    saveResultAsLAS(target_sampled, "sampled_target.las");
    
    // 根据点云大小估计运行时间
    size_t total_operations = source_sampled.size() * target_sampled.size() * 20;  // 估算
    if (total_operations > 100000000) {
        cout << "\n警告: 点云较大,配准可能需要较长时间!" << endl;
        cout << "建议: 增加 sample_rate 以减少点数,或减少 max_iterations" << endl;
    }
    
    // 执行ICP配准
    int max_iters = 20;  // 迭代次数
    double tolerance = 1e-5;  // 收敛阈值
    
    cout << "\n配准参数: 最大迭代次数=" << max_iters << ", 收敛阈值=" << tolerance << endl;
    
    // 用于存储变换参数和迭代过程
    double final_R[3][3], final_t[3];
    vector<Eigen::Matrix4d> iteration_transforms;
    
    cout << "\n说明: ICP算法将 源点云(096) 配准到 目标点云(099)" << endl;
    
    ICP(source_sampled, target_sampled, max_iters, tolerance, final_R, final_t, &iteration_transforms);
    
    // 输出变换参数
    cout << "\n========== 配准变换参数 ==========" << endl;
    cout << "旋转矩阵 R:" << endl;
    for (int i = 0; i < 3; i++) {
        cout << "  [";
        for (int j = 0; j < 3; j++) {
            cout << final_R[i][j];
            if (j < 2) cout << ", ";
        }
        cout << "]" << endl;
    }
    cout << "\n平移向量 t:" << endl;
    cout << "  [" << final_t[0] << ", " << final_t[1] << ", " << final_t[2] << "]" << endl;
    cout << "================================" << endl;
    
    // 保存结果
    cout << "\n保存配准结果..." << endl;
    saveResultAsLAS(source_sampled, "registered_source.las");
    saveResultAsLAS(target_sampled, "registered_target.las");
    
    // 保存变换参数（包含迭代过程）
    saveTransformation(final_R, final_t, "icp_transformation.txt", &iteration_transforms);
    
    cout << "\n配准完成!" << endl;
    cout << "已生成文件:" << endl;
    cout << "  - registered_source.las (配准后的源点云)" << endl;
    cout << "  - registered_target.las (目标点云参考)" << endl;
    cout << "  - icp_transformation.txt (变换参数，含每次迭代)" << endl;
    
    cout << "\n请按回车键退出..." << endl;
    cin.clear();
    cin.ignore(10000, '\n');
    cin.get();
    
    return 0;
}
