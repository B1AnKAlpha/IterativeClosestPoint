#include "registrationpage.h"
#include "ElaPushButton.h"
#include "ElaProgressBar.h"
#include "ElaText.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>

RegistrationPage::RegistrationPage(QWidget *parent)
    : QWidget(parent)
    , m_registrationService(nullptr)
    , m_settingsService(nullptr)
{
    buildUI();
}

void RegistrationPage::setRegistrationService(RegistrationService* service)
{
    m_registrationService = service;
    
    if (m_registrationService) {
        connect(m_registrationService, &RegistrationService::registrationStarted,
                this, &RegistrationPage::onRegistrationStarted);
        connect(m_registrationService, &RegistrationService::registrationProgress,
                this, &RegistrationPage::onRegistrationProgress);
        connect(m_registrationService, &RegistrationService::registrationIterationCompleted,
                this, &RegistrationPage::onIterationCompleted);
        connect(m_registrationService, &RegistrationService::registrationFinished,
                this, &RegistrationPage::onRegistrationFinished);
        connect(m_registrationService, &RegistrationService::registrationLog,
                this, &RegistrationPage::onLogMessage);
    }
}

void RegistrationPage::setSettingsService(SettingsService* service)
{
    m_settingsService = service;
}

void RegistrationPage::buildUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // 标题
    ElaText* titleText = new ElaText("配准控制台", this);
    titleText->setTextPixelSize(24);
    mainLayout->addWidget(titleText);
    
    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_startButton = new ElaPushButton("开始配准", this);
    m_stopButton = new ElaPushButton("停止配准", this);
    m_stopButton->setEnabled(false);
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // 进度组
    QGroupBox* progressGroup = new QGroupBox("配准进度");
    QVBoxLayout* progressLayout = new QVBoxLayout();
    
    m_progressBar = new ElaProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    progressLayout->addWidget(m_progressBar);
    
    QHBoxLayout* infoLayout = new QHBoxLayout();
    m_iterationLabel = new QLabel("迭代: 0/0");
    m_rmseLabel = new QLabel("RMSE: -");
    infoLayout->addWidget(m_iterationLabel);
    infoLayout->addWidget(m_rmseLabel);
    infoLayout->addStretch();
    
    progressLayout->addLayout(infoLayout);
    
    QHBoxLayout* pointsLayout = new QHBoxLayout();
    m_validPointsLabel = new QLabel("有效点: -");
    m_outlierPointsLabel = new QLabel("离群点: -");
    pointsLayout->addWidget(m_validPointsLabel);
    pointsLayout->addWidget(m_outlierPointsLabel);
    pointsLayout->addStretch();
    
    progressLayout->addLayout(pointsLayout);
    
    progressGroup->setLayout(progressLayout);
    mainLayout->addWidget(progressGroup);
    
    // 日志输出
    QGroupBox* logGroup = new QGroupBox("日志输出");
    QVBoxLayout* logLayout = new QVBoxLayout();
    
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setMaximumHeight(200);
    logLayout->addWidget(m_logTextEdit);
    
    logGroup->setLayout(logLayout);
    mainLayout->addWidget(logGroup);
    
    // 迭代结果表格
    QGroupBox* resultGroup = new QGroupBox("迭代详情");
    QVBoxLayout* resultLayout = new QVBoxLayout();
    
    m_resultTable = new QTableWidget(this);
    m_resultTable->setColumnCount(4);
    m_resultTable->setHorizontalHeaderLabels({"迭代", "RMSE", "有效点", "离群点"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->setMaximumHeight(150);
    resultLayout->addWidget(m_resultTable);
    
    resultGroup->setLayout(resultLayout);
    mainLayout->addWidget(resultGroup);
    
    // 连接信号
    connect(m_startButton, &ElaPushButton::clicked, this, &RegistrationPage::onStartRegistration);
    connect(m_stopButton, &ElaPushButton::clicked, this, &RegistrationPage::onStopRegistration);
}

void RegistrationPage::onStartRegistration()
{
    if (!m_registrationService) {
        QMessageBox::warning(this, "错误", "配准服务未初始化");
        return;
    }
    
    if (!m_registrationService->getSourceCloud() || !m_registrationService->getTargetCloud()) {
        QMessageBox::warning(this, "错误", "请先加载源点云和目标点云");
        return;
    }
    
    // 清空之前的日志和结果
    m_logTextEdit->clear();
    m_resultTable->setRowCount(0);
    m_progressBar->setValue(0);
    
    // 获取当前参数
    ICPParameters params;
    if (m_settingsService) {
        params = m_settingsService->getICPParameters();
    }
    
    appendLog(QString("开始配准，参数设置："));
    appendLog(QString("  最大迭代次数: %1").arg(params.maxIterations));
    appendLog(QString("  收敛容差: %1").arg(params.tolerance));
    appendLog(QString("  Sigma倍数: %1").arg(params.sigmaMultiplier));
    
    m_registrationService->startRegistration(params);
}

void RegistrationPage::onStopRegistration()
{
    if (m_registrationService) {
        m_registrationService->stopRegistration();
    }
}

void RegistrationPage::onRegistrationStarted()
{
    updateControls(true);
    appendLog("配准已启动...");
}

void RegistrationPage::onRegistrationProgress(int iteration, int total, double rmse)
{
    int progress = (total > 0) ? (iteration * 100 / total) : 0;
    m_progressBar->setValue(progress);
    
    m_iterationLabel->setText(QString("迭代: %1/%2").arg(iteration).arg(total));
    m_rmseLabel->setText(QString("RMSE: %1").arg(rmse, 0, 'f', 6));
}

void RegistrationPage::onIterationCompleted(const IterationResult& result)
{
    // 添加到结果表格
    int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);
    
    m_resultTable->setItem(row, 0, new QTableWidgetItem(QString::number(result.iteration)));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(QString::number(result.rmse, 'f', 6)));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(QString::number(result.validPoints)));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(QString::number(result.outlierPoints)));
    
    // 滚动到最新行
    m_resultTable->scrollToBottom();
    
    // 更新统计信息
    m_validPointsLabel->setText(QString("有效点: %1").arg(result.validPoints));
    m_outlierPointsLabel->setText(QString("离群点: %1").arg(result.outlierPoints));
}

void RegistrationPage::onRegistrationFinished(bool success, const QString& message)
{
    updateControls(false);
    
    if (success) {
        appendLog(QString("✓ 配准成功: %1").arg(message));
        QMessageBox::information(this, "成功", "点云配准完成！\n您可以在可视化页面查看结果。");
    } else {
        appendLog(QString("✗ 配准失败: %1").arg(message));
        QMessageBox::warning(this, "失败", message);
    }
    
    m_progressBar->setValue(100);
}

void RegistrationPage::onLogMessage(const QString& message)
{
    appendLog(message);
}

void RegistrationPage::updateControls(bool isRegistering)
{
    m_startButton->setEnabled(!isRegistering);
    m_stopButton->setEnabled(isRegistering);
}

void RegistrationPage::appendLog(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logTextEdit->append(QString("[%1] %2").arg(timestamp).arg(message));
}
