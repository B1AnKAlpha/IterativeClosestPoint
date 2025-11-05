#ifndef REGISTRATIONSERVICE_H
#define REGISTRATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QFutureWatcher>
#include "core/pointcloud.h"
#include "core/icpengine.h"

/**
 * @brief 配准历史记录
 */
struct RegistrationRecord {
    QDateTime timestamp;
    QString sourceFile;
    QString targetFile;
    int sourcePoints;
    int targetPoints;
    int iterations;
    double finalRMSE;
    bool success;
};

/**
 * @brief 配准服务
 * 
 * 管理点云数据和配准流程
 */
class RegistrationService : public QObject
{
    Q_OBJECT

public:
    explicit RegistrationService(QObject *parent = nullptr);
    ~RegistrationService() override;
    
    // 点云管理
    bool loadSourceCloud(const QString& filename, size_t maxPoints = 0);
    bool loadTargetCloud(const QString& filename, size_t maxPoints = 0);
    bool saveRegisteredCloud(const QString& filename);
    
    void clearSourceCloud();
    void clearTargetCloud();
    
    PointCloud* getSourceCloud() { return m_sourceCloud; }
    const PointCloud* getTargetCloud() const { return m_targetCloud; }
    
    // 获取原始源点云(配准前的状态)
    const PointCloud* getOriginalSourceCloud() const { return m_originalSourceCloud; }
    
    QString getSourceFile() const { return m_sourceFile; }
    QString getTargetFile() const { return m_targetFile; }
    
    // 配准操作
    void startRegistration(const ICPParameters& params);
    void stopRegistration();
    bool isRegistering() const { return m_isRegistering; }
    
    // 获取ICP迭代历史
    std::vector<IterationResult> getIterationHistory() const;
    
    // 历史记录
    const QVector<RegistrationRecord>& getHistory() const { return m_history; }
    void clearHistory();
    
signals:
    void sourceCloudLoaded(const QString& filename, int pointCount);
    void targetCloudLoaded(const QString& filename, int pointCount);
    void cloudLoadError(const QString& message);
    void cloudLoadProgress(const QString& message);  // 新增：加载进度信号
    
    void registrationStarted();
    void registrationProgress(int iteration, int total, double rmse);
    void registrationIterationCompleted(const IterationResult& result);
    void registrationFinished(bool success, const QString& message);
    void registrationLog(const QString& message);
    
    void historyUpdated(const QVector<RegistrationRecord>& history);
    
private slots:
    void onICPFinished(bool success, const QString& message);
    void onSourceCloudLoadFinished();
    void onTargetCloudLoadFinished();
    void onRegistrationFinished();
    
private:
    PointCloud* m_sourceCloud;
    PointCloud* m_targetCloud;
    PointCloud* m_originalSourceCloud;  // 保存原始源点云用于迭代回放
    ICPEngine* m_icpEngine;
    
    QString m_sourceFile;
    QString m_targetFile;
    
    bool m_isRegistering;
    QVector<RegistrationRecord> m_history;
    
    // 异步加载
    QFutureWatcher<PointCloud*>* m_sourceWatcher;
    QFutureWatcher<PointCloud*>* m_targetWatcher;
    
    // 异步配准
    QFutureWatcher<void>* m_registrationWatcher;
};

#endif // REGISTRATIONSERVICE_H
