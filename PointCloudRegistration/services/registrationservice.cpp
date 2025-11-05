#include "registrationservice.h"
#include "core/lasio.h"
#include <QFileInfo>
#include <QDebug>
#include <QtConcurrent>

RegistrationService::RegistrationService(QObject *parent)
    : QObject(parent)
    , m_sourceCloud(nullptr)
    , m_targetCloud(nullptr)
    , m_originalSourceCloud(nullptr)
    , m_isRegistering(false)
    , m_sourceWatcher(nullptr)
    , m_targetWatcher(nullptr)
    , m_registrationWatcher(nullptr)
{
    m_icpEngine = new ICPEngine(this);
    
    // 连接ICP引擎信号
    connect(m_icpEngine, &ICPEngine::started, this, &RegistrationService::registrationStarted);
    connect(m_icpEngine, &ICPEngine::progressUpdated, this, &RegistrationService::registrationProgress);
    connect(m_icpEngine, &ICPEngine::iterationCompleted, this, &RegistrationService::registrationIterationCompleted);
    connect(m_icpEngine, &ICPEngine::finished, this, &RegistrationService::onICPFinished);
    connect(m_icpEngine, &ICPEngine::logMessage, this, &RegistrationService::registrationLog);
    
    // 创建异步加载的Watcher
    m_sourceWatcher = new QFutureWatcher<PointCloud*>(this);
    m_targetWatcher = new QFutureWatcher<PointCloud*>(this);
    
    connect(m_sourceWatcher, &QFutureWatcher<PointCloud*>::finished,
            this, &RegistrationService::onSourceCloudLoadFinished);
    connect(m_targetWatcher, &QFutureWatcher<PointCloud*>::finished,
            this, &RegistrationService::onTargetCloudLoadFinished);
    
    // 创建异步配准的Watcher
    m_registrationWatcher = new QFutureWatcher<void>(this);
    connect(m_registrationWatcher, &QFutureWatcher<void>::finished,
            this, &RegistrationService::onRegistrationFinished);
}

RegistrationService::~RegistrationService()
{
    if (m_sourceCloud) delete m_sourceCloud;
    if (m_targetCloud) delete m_targetCloud;
    if (m_originalSourceCloud) delete m_originalSourceCloud;
}

bool RegistrationService::loadSourceCloud(const QString& filename, size_t maxPoints)
{
    if (m_sourceWatcher->isRunning()) {
        emit cloudLoadError("源点云正在加载中,请稍候...");
        return false;
    }
    
    if (m_sourceCloud) {
        delete m_sourceCloud;
        m_sourceCloud = nullptr;
    }
    
    m_sourceFile = filename;
    emit cloudLoadProgress("正在加载源点云，请稍候...");
    
    // 异步加载
    auto loadFunc = [filename, maxPoints]() -> PointCloud* {
        PointCloud* cloud = new PointCloud();
        cloud->color = QColor(255, 100, 100);  // 红色
        
        if (!LASIO::readLAS(filename.toStdString(), *cloud, maxPoints)) {
            delete cloud;
            return nullptr;
        }
        
        return cloud;
    };
    
    QFuture<PointCloud*> future = QtConcurrent::run(loadFunc);
    m_sourceWatcher->setFuture(future);
    
    return true;
}

void RegistrationService::onSourceCloudLoadFinished()
{
    m_sourceCloud = m_sourceWatcher->result();
    
    if (!m_sourceCloud) {
        emit cloudLoadError("无法加载源点云: " + m_sourceFile);
        return;
    }
    
    // 保存原始源点云的副本
    if (m_originalSourceCloud) {
        delete m_originalSourceCloud;
    }
    m_originalSourceCloud = new PointCloud();
    m_originalSourceCloud->points = m_sourceCloud->points;
    m_originalSourceCloud->color = m_sourceCloud->color;
    m_originalSourceCloud->computeBounds();
    
    emit sourceCloudLoaded(m_sourceFile, static_cast<int>(m_sourceCloud->size()));
}

bool RegistrationService::loadTargetCloud(const QString& filename, size_t maxPoints)
{
    if (m_targetWatcher->isRunning()) {
        emit cloudLoadError("目标点云正在加载中,请稍候...");
        return false;
    }
    
    if (m_targetCloud) {
        delete m_targetCloud;
        m_targetCloud = nullptr;
    }
    
    m_targetFile = filename;
    emit cloudLoadProgress("正在加载目标点云，请稍候...");
    
    // 异步加载
    auto loadFunc = [filename, maxPoints]() -> PointCloud* {
        PointCloud* cloud = new PointCloud();
        cloud->color = QColor(100, 100, 255);  // 蓝色
        
        if (!LASIO::readLAS(filename.toStdString(), *cloud, maxPoints)) {
            delete cloud;
            return nullptr;
        }
        
        return cloud;
    };
    
    QFuture<PointCloud*> future = QtConcurrent::run(loadFunc);
    m_targetWatcher->setFuture(future);
    
    return true;
}

void RegistrationService::onTargetCloudLoadFinished()
{
    m_targetCloud = m_targetWatcher->result();
    
    if (!m_targetCloud) {
        emit cloudLoadError("无法加载目标点云: " + m_targetFile);
        return;
    }
    
    emit targetCloudLoaded(m_targetFile, static_cast<int>(m_targetCloud->size()));
}

bool RegistrationService::saveRegisteredCloud(const QString& filename)
{
    if (!m_sourceCloud || m_sourceCloud->empty()) {
        emit cloudLoadError("没有可保存的点云数据");
        return false;
    }
    
    if (!LASIO::writeLAS(filename.toStdString(), *m_sourceCloud)) {
        emit cloudLoadError("无法保存点云: " + filename);
        return false;
    }
    
    return true;
}

void RegistrationService::clearSourceCloud()
{
    if (m_sourceCloud) {
        delete m_sourceCloud;
        m_sourceCloud = nullptr;
    }
    if (m_originalSourceCloud) {
        delete m_originalSourceCloud;
        m_originalSourceCloud = nullptr;
    }
    m_sourceFile.clear();
}

void RegistrationService::clearTargetCloud()
{
    if (m_targetCloud) {
        delete m_targetCloud;
        m_targetCloud = nullptr;
    }
    m_targetFile.clear();
}

void RegistrationService::startRegistration(const ICPParameters& params)
{
    if (m_isRegistering) {
        emit registrationLog("配准正在进行中...");
        return;
    }
    
    if (!m_sourceCloud || !m_targetCloud) {
        emit registrationFinished(false, "请先加载源点云和目标点云");
        return;
    }
    
    if (m_sourceCloud->empty() || m_targetCloud->empty()) {
        emit registrationFinished(false, "点云数据为空");
        return;
    }
    
    m_isRegistering = true;
    m_icpEngine->setParameters(params);
    
    // 在后台线程中运行配准
    auto registrationFunc = [this]() {
        m_icpEngine->registerPointClouds(m_sourceCloud, m_targetCloud);
    };
    
    QFuture<void> future = QtConcurrent::run(registrationFunc);
    m_registrationWatcher->setFuture(future);
}

void RegistrationService::stopRegistration()
{
    if (m_isRegistering) {
        m_icpEngine->stop();
    }
}

std::vector<IterationResult> RegistrationService::getIterationHistory() const
{
    return m_icpEngine->getResult().iterationHistory;
}

void RegistrationService::onICPFinished(bool success, const QString& message)
{
    // ICP引擎完成后的处理 - 这在工作线程中被调用
    // 实际的结束处理在onRegistrationFinished中进行
}

void RegistrationService::onRegistrationFinished()
{
    m_isRegistering = false;
    
    // 获取ICP结果
    ICPResult result = m_icpEngine->getResult();
    bool success = result.success;
    
    // 添加到历史记录
    if (success) {
        RegistrationRecord record;
        record.timestamp = QDateTime::currentDateTime();
        record.sourceFile = QFileInfo(m_sourceFile).fileName();
        record.targetFile = QFileInfo(m_targetFile).fileName();
        record.sourcePoints = static_cast<int>(m_sourceCloud->size());
        record.targetPoints = static_cast<int>(m_targetCloud->size());
        record.iterations = result.totalIterations;
        record.finalRMSE = result.finalRMSE;
        record.success = true;
        
        m_history.append(record);
        emit historyUpdated(m_history);
    }
    
    QString message = success ? 
        QString("配准成功! RMSE: %1, 迭代次数: %2")
            .arg(result.finalRMSE, 0, 'f', 6)
            .arg(result.totalIterations) :
        "配准失败";
    
    emit registrationFinished(success, message);
}

void RegistrationService::clearHistory()
{
    m_history.clear();
    emit historyUpdated(m_history);
}
