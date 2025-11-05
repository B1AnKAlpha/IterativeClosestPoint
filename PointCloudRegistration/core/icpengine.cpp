#include "icpengine.h"
#include "octree.h"
#include <cmath>
#include <numeric>
#include <QDebug>

ICPEngine::ICPEngine(QObject *parent)
    : QObject(parent)
    , m_source(nullptr)
    , m_target(nullptr)
    , m_shouldStop(false)
{
}

ICPEngine::~ICPEngine()
{
}

void ICPEngine::setParameters(const ICPParameters& params)
{
    m_params = params;
}

void ICPEngine::registerPointClouds(PointCloud* source, const PointCloud* target)
{
    if (!source || !target) {
        emit finished(false, "源点云或目标点云为空");
        return;
    }
    
    if (source->empty() || target->empty()) {
        emit finished(false, "点云数据为空");
        return;
    }
    
    m_source = source;
    m_target = target;
    m_shouldStop = false;
    m_result = ICPResult();
    m_result.iterationHistory.clear();
    
    emit started();
    emit logMessage("========== 开始ICP配准 ==========");
    emit logMessage(QString("源点云: %1 个点").arg(source->size()));
    emit logMessage(QString("目标点云: %1 个点").arg(target->size()));
    
    // 检查点云数据有效性
    if (!source->points.empty()) {
        const Point3D& p = source->points[0];
        emit logMessage(QString("源点云第一个点: (%1, %2, %3)")
                       .arg(p.x, 0, 'f', 3).arg(p.y, 0, 'f', 3).arg(p.z, 0, 'f', 3));
    }
    if (!target->points.empty()) {
        const Point3D& p = target->points[0];
        emit logMessage(QString("目标点云第一个点: (%1, %2, %3)")
                       .arg(p.x, 0, 'f', 3).arg(p.y, 0, 'f', 3).arg(p.z, 0, 'f', 3));
    }
    
    runICP();
}

void ICPEngine::stop()
{
    m_shouldStop = true;
    emit logMessage("用户请求停止配准...");
}

double ICPEngine::computeDistance(const Point3D& p1, const Point3D& p2) const
{
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    double dz = p1.z - p2.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

Eigen::Matrix4d ICPEngine::computeBestFitTransform(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B)
{
    // A和B是3xN矩阵
    int N = A.cols();
    
    // 计算质心
    Eigen::Vector3d centroid_A = A.rowwise().mean();
    Eigen::Vector3d centroid_B = B.rowwise().mean();
    
    // 去中心化
    Eigen::MatrixXd AA = A.colwise() - centroid_A;
    Eigen::MatrixXd BB = B.colwise() - centroid_B;
    
    // 计算协方差矩阵
    Eigen::Matrix3d H = AA * BB.transpose();
    
    // SVD分解
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Matrix3d U = svd.matrixU();
    Eigen::Matrix3d V = svd.matrixV();
    
    // 计算旋转矩阵
    Eigen::Matrix3d R = V * U.transpose();
    
    // 处理反射情况
    if (R.determinant() < 0) {
        V.col(2) *= -1;
        R = V * U.transpose();
    }
    
    // 计算平移向量
    Eigen::Vector3d t = centroid_B - R * centroid_A;
    
    // 构建4x4变换矩阵
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3,3>(0,0) = R;
    T.block<3,1>(0,3) = t;
    
    return T;
}

void ICPEngine::runICP()
{
    emit logMessage("构建目标点云八叉树索引...");
    
    // 构建八叉树
    Octree octree(m_target->points, m_params.octreeMaxPoints, m_params.octreeMaxDepth);
    
    emit logMessage("八叉树构建完成!");
    
    // 测试八叉树查询
    if (!m_source->points.empty() && !m_target->points.empty()) {
        Point3D test_query = m_source->points[0];
        int test_idx = octree.findNearest(test_query);
        const Point3D& test_result = m_target->points[test_idx];
        double test_dist = computeDistance(test_query, test_result);
        emit logMessage(QString("八叉树测试: 查询点(%1,%2,%3) -> 最近点[%4](%5,%6,%7), 距离=%8")
                       .arg(test_query.x, 0, 'f', 3).arg(test_query.y, 0, 'f', 3).arg(test_query.z, 0, 'f', 3)
                       .arg(test_idx)
                       .arg(test_result.x, 0, 'f', 3).arg(test_result.y, 0, 'f', 3).arg(test_result.z, 0, 'f', 3)
                       .arg(test_dist, 0, 'f', 3));
    }
    
    int row = static_cast<int>(m_source->size());
    
    // 转换为Eigen矩阵
    Eigen::MatrixXd src = Eigen::MatrixXd::Ones(4, row);
    Eigen::MatrixXd src3d = Eigen::MatrixXd::Ones(3, row);
    
    for (int i = 0; i < row; i++) {
        src3d(0, i) = m_source->points[i].x;
        src3d(1, i) = m_source->points[i].y;
        src3d(2, i) = m_source->points[i].z;
        src(0, i) = m_source->points[i].x;
        src(1, i) = m_source->points[i].y;
        src(2, i) = m_source->points[i].z;
    }
    
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d T_cumulative = Eigen::Matrix4d::Identity();
    double prev_error = 1e10;
    int no_improvement_count = 0;
    
    for (int iter = 0; iter < m_params.maxIterations; iter++) {
        if (m_shouldStop) {
            emit logMessage("配准已停止");
            emit finished(false, "用户取消");
            return;
        }
        
        emit logMessage(QString("迭代 %1/%2 ...").arg(iter + 1).arg(m_params.maxIterations));
        
        // 步骤1: 使用八叉树找到最近点对应
        std::vector<int> correspondences(row);
        Eigen::MatrixXd dst_matched = Eigen::MatrixXd::Ones(3, row);
        
        for (int i = 0; i < row; i++) {
            Point3D query;
            query.x = src3d(0, i);
            query.y = src3d(1, i);
            query.z = src3d(2, i);
            
            int nearest_idx = octree.findNearest(query);
            correspondences[i] = nearest_idx;
            
            dst_matched(0, i) = m_target->points[nearest_idx].x;
            dst_matched(1, i) = m_target->points[nearest_idx].y;
            dst_matched(2, i) = m_target->points[nearest_idx].z;
        }
        
        // 步骤2: 计算所有点对的距离
        std::vector<double> distances(row);
        double min_distance = std::numeric_limits<double>::max();
        double max_distance = 0;
        int problem_count = 0;
        
        for (int i = 0; i < row; i++) {
            Point3D p_src;
            p_src.x = src3d(0, i);
            p_src.y = src3d(1, i);
            p_src.z = src3d(2, i);
            
            int nearest_idx = correspondences[i];
            if (nearest_idx < 0 || nearest_idx >= static_cast<int>(m_target->points.size())) {
                emit logMessage(QString("警告: 索引越界 i=%1, nearest_idx=%2").arg(i).arg(nearest_idx));
                problem_count++;
                distances[i] = std::numeric_limits<double>::max();
                continue;
            }
            
            distances[i] = computeDistance(p_src, m_target->points[nearest_idx]);
            
            if (std::isnan(distances[i]) || std::isinf(distances[i])) {
                if (problem_count < 5) {  // 只打印前5个问题
                    emit logMessage(QString("警告: 距离异常 i=%1, dist=%2, src=(%3,%4,%5), tgt=(%6,%7,%8)")
                                   .arg(i).arg(distances[i])
                                   .arg(p_src.x).arg(p_src.y).arg(p_src.z)
                                   .arg(m_target->points[nearest_idx].x)
                                   .arg(m_target->points[nearest_idx].y)
                                   .arg(m_target->points[nearest_idx].z));
                }
                problem_count++;
            }
            
            if (std::isfinite(distances[i])) {
                if (distances[i] < min_distance) min_distance = distances[i];
                if (distances[i] > max_distance) max_distance = distances[i];
            }
        }
        
        if (problem_count > 0) {
            emit logMessage(QString("警告: 发现 %1 个异常距离值").arg(problem_count));
        }
        
        emit logMessage(QString("  距离范围: 最小=%1, 最大=%2")
                       .arg(min_distance, 0, 'f', 6)
                       .arg(max_distance, 0, 'f', 6));
        
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
        double std_dev = std::sqrt(variance / row);
        
        // 使用3-sigma原则设置距离阈值剔除离群点
        // 第一次迭代时,如果标准差很小(点云很密集),使用更大的阈值
        double threshold;
        if (iter == 0) {
            // 第一次迭代:使用较宽松的阈值,防止过度剔除
            threshold = mean_dist + std::max(m_params.sigmaMultiplier * std_dev, mean_dist * 0.5);
        } else {
            threshold = mean_dist + m_params.sigmaMultiplier * std_dev;
        }
        
        emit logMessage(QString("  距离统计: 均值=%1, 标准差=%2, 阈值=%3")
                       .arg(mean_dist, 0, 'f', 6)
                       .arg(std_dev, 0, 'f', 6)
                       .arg(threshold, 0, 'f', 6));
        
        // 统计有效点对
        std::vector<int> valid_indices;
        for (int i = 0; i < row; i++) {
            if (distances[i] <= threshold) {
                valid_indices.push_back(i);
            }
        }
        
        int valid_count = static_cast<int>(valid_indices.size());
        int outlier_count = row - valid_count;
        
        // 只用有效点计算RMSE
        double sum_sq = 0;
        for (int idx : valid_indices) {
            sum_sq += distances[idx] * distances[idx];
        }
        double mean_error = (valid_count > 0) ? std::sqrt(sum_sq / valid_count) : 0;
        
        emit logMessage(QString("  RMSE = %1 (有效点: %2/%3, 剔除离群点: %4)")
                       .arg(mean_error, 0, 'f', 6)
                       .arg(valid_count)
                       .arg(row)
                       .arg(outlier_count));
        
        // 步骤3: 检查收敛
        double improvement = prev_error - mean_error;
        if (std::abs(improvement) < m_params.tolerance) {
            no_improvement_count++;
            if (no_improvement_count >= 3) {
                emit logMessage(QString("收敛达到! 迭代次数: %1").arg(iter + 1));
                
                // 记录最后一次迭代
                IterationResult iterResult;
                iterResult.iteration = iter + 1;
                iterResult.rmse = mean_error;
                iterResult.validPoints = valid_count;
                iterResult.outlierPoints = outlier_count;
                iterResult.transform = T_cumulative;
                
                m_result.iterationHistory.push_back(iterResult);
                emit iterationCompleted(iterResult);
                emit progressUpdated(iter + 1, m_params.maxIterations, mean_error);
                
                break;
            }
        } else {
            no_improvement_count = 0;
        }
        
        if (mean_error > prev_error * 1.1) {
            emit logMessage("警告: 误差增加，停止迭代");
            break;
        }
        
        prev_error = mean_error;
        
        // 步骤4: 使用SVD计算最佳变换（只使用非离群点）
        if (valid_count < 3) {
            emit logMessage("错误: 有效点对不足，无法计算变换");
            emit finished(false, "有效点对不足");
            return;
        }
        
        Eigen::MatrixXd src_valid(3, valid_count);
        Eigen::MatrixXd dst_valid(3, valid_count);
        
        for (int i = 0; i < valid_count; i++) {
            int idx = valid_indices[i];
            src_valid(0, i) = src3d(0, idx);
            src_valid(1, i) = src3d(1, idx);
            src_valid(2, i) = src3d(2, idx);
            
            dst_valid(0, i) = dst_matched(0, idx);
            dst_valid(1, i) = dst_matched(1, idx);
            dst_valid(2, i) = dst_matched(2, idx);
        }
        
        T = computeBestFitTransform(src_valid, dst_valid);
        
        // 累积变换矩阵
        T_cumulative = T * T_cumulative;
        
        // 应用变换
        src = T * src;
        src3d = src.topRows(3);
        
        // 记录迭代结果
        IterationResult iterResult;
        iterResult.iteration = iter + 1;
        iterResult.rmse = mean_error;
        iterResult.validPoints = valid_count;
        iterResult.outlierPoints = outlier_count;
        iterResult.transform = T_cumulative;
        
        // 计算旋转角度和平移距离
        Eigen::Matrix3d R = T_cumulative.block<3,3>(0,0);
        Eigen::Vector3d t = T_cumulative.block<3,1>(0,3);
        
        double trace = R.trace();
        iterResult.rotationAngle = std::acos((trace - 1.0) / 2.0) * 180.0 / M_PI;
        iterResult.translationDistance = t.norm();
        
        m_result.iterationHistory.push_back(iterResult);
        
        emit iterationCompleted(iterResult);
        emit progressUpdated(iter + 1, m_params.maxIterations, mean_error);
    }
    
    // 将结果写回源点云
    for (int i = 0; i < row; i++) {
        m_source->points[i].x = src3d(0, i);
        m_source->points[i].y = src3d(1, i);
        m_source->points[i].z = src3d(2, i);
    }
    
    // 提取最终的旋转矩阵和平移向量
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m_result.finalR[i][j] = T_cumulative(i, j);
        }
        m_result.finalT[i] = T_cumulative(i, 3);
    }
    
    m_result.success = true;
    m_result.totalIterations = static_cast<int>(m_result.iterationHistory.size());
    m_result.finalRMSE = m_result.iterationHistory.empty() ? 0.0 : m_result.iterationHistory.back().rmse;
    
    emit logMessage("========== 配准完成 ==========");
    emit logMessage(QString("总迭代次数: %1").arg(m_result.totalIterations));
    emit logMessage(QString("最终RMSE: %1").arg(m_result.finalRMSE, 0, 'f', 6));
    
    emit finished(true, "配准成功");
}
