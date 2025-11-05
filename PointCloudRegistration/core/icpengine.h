#ifndef ICPENGINE_H
#define ICPENGINE_H

#include <QObject>
#include <QThread>
#include <vector>
#include "pointcloud.h"
#include "Eigen/Eigen"

/**
 * @brief ICP配准参数
 */
struct ICPParameters {
    int maxIterations = 50;           // 最大迭代次数
    double tolerance = 1e-6;          // 收敛容差
    double sigmaMultiplier = 3.0;     // 3-sigma阈值倍数
    int octreeMaxPoints = 10;         // 八叉树每节点最大点数
    int octreeMaxDepth = 20;          // 八叉树最大深度
};

/**
 * @brief 单次迭代结果
 */
struct IterationResult {
    int iteration;                    // 迭代次数
    double rmse;                      // RMSE值
    int validPoints;                  // 有效点对数
    int outlierPoints;                // 离群点数
    Eigen::Matrix4d transform;        // 累积变换矩阵
    double rotationAngle;             // 旋转角度（度）
    double translationDistance;       // 平移距离
};

/**
 * @brief ICP配准结果
 */
struct ICPResult {
    bool success;                     // 是否成功
    int totalIterations;              // 总迭代次数
    double finalRMSE;                 // 最终RMSE
    double finalR[3][3];              // 最终旋转矩阵
    double finalT[3];                 // 最终平移向量
    std::vector<IterationResult> iterationHistory;  // 迭代历史
};

/**
 * @brief ICP配准引擎
 * 
 * 封装ICP算法，提供Qt信号接口用于进度报告
 */
class ICPEngine : public QObject
{
    Q_OBJECT

public:
    explicit ICPEngine(QObject *parent = nullptr);
    ~ICPEngine() override;
    
    // 设置参数
    void setParameters(const ICPParameters& params);
    ICPParameters getParameters() const { return m_params; }
    
    // 配准接口
    void registerPointClouds(PointCloud* source, const PointCloud* target);
    void stop();
    
    // 获取结果
    ICPResult getResult() const { return m_result; }
    
signals:
    void started();
    void progressUpdated(int iteration, int total, double rmse);
    void iterationCompleted(const IterationResult& result);
    void finished(bool success, const QString& message);
    void logMessage(const QString& message);
    
private:
    void runICP();
    double computeDistance(const Point3D& p1, const Point3D& p2) const;
    Eigen::Matrix4d computeBestFitTransform(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B);
    
    ICPParameters m_params;
    PointCloud* m_source;
    const PointCloud* m_target;
    ICPResult m_result;
    bool m_shouldStop;
};

#endif // ICPENGINE_H
