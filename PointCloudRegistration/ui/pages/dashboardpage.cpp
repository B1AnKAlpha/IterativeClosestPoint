#include "dashboardpage.h"
#include "ElaText.h"
#include "ElaTableView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStandardItemModel>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
    , m_registrationService(nullptr)
{
    buildUI();
}

void DashboardPage::setRegistrationService(RegistrationService* service)
{
    m_registrationService = service;
    
    if (m_registrationService) {
        connect(m_registrationService, &RegistrationService::sourceCloudLoaded,
                this, &DashboardPage::updateCloudInfo);
        connect(m_registrationService, &RegistrationService::targetCloudLoaded,
                this, &DashboardPage::updateCloudInfo);
        connect(m_registrationService, &RegistrationService::historyUpdated,
                this, &DashboardPage::updateHistory);
    }
}

void DashboardPage::buildUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);
    
    // 标题和欢迎文本
    ElaText* titleText = new ElaText("点云配准系统", this);
    titleText->setTextPixelSize(28);
    mainLayout->addWidget(titleText);
    
    ElaText* welcomeText = new ElaText("欢迎使用ICP点云配准软件", this);
    welcomeText->setTextPixelSize(16);
    mainLayout->addWidget(welcomeText);
    
    // 信息卡片行
    QHBoxLayout* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(15);
    
    // 源点云卡片
    QGroupBox* sourceCard = new QGroupBox("源点云");
    QVBoxLayout* sourceLayout = new QVBoxLayout();
    m_sourcePointsValue = new QLabel("0");
    m_sourcePointsValue->setAlignment(Qt::AlignCenter);
    m_sourcePointsValue->setStyleSheet("font-size: 24px; font-weight: bold;");
    QLabel* sourceLabel = new QLabel("点数");
    sourceLabel->setAlignment(Qt::AlignCenter);
    
    sourceLayout->addWidget(m_sourcePointsValue);
    sourceLayout->addWidget(sourceLabel);
    sourceCard->setLayout(sourceLayout);
    cardsLayout->addWidget(sourceCard);
    
    // 目标点云卡片
    QGroupBox* targetCard = new QGroupBox("目标点云");
    QVBoxLayout* targetLayout = new QVBoxLayout();
    m_targetPointsValue = new QLabel("0");
    m_targetPointsValue->setAlignment(Qt::AlignCenter);
    m_targetPointsValue->setStyleSheet("font-size: 24px; font-weight: bold;");
    QLabel* targetLabel = new QLabel("点数");
    targetLabel->setAlignment(Qt::AlignCenter);
    
    targetLayout->addWidget(m_targetPointsValue);
    targetLayout->addWidget(targetLabel);
    targetCard->setLayout(targetLayout);
    cardsLayout->addWidget(targetCard);
    
    // 配准次数卡片
    QGroupBox* registrationCard = new QGroupBox("配准历史");
    QVBoxLayout* registrationLayout = new QVBoxLayout();
    m_registrationCountValue = new QLabel("0");
    m_registrationCountValue->setAlignment(Qt::AlignCenter);
    m_registrationCountValue->setStyleSheet("font-size: 24px; font-weight: bold;");
    QLabel* registrationLabel = new QLabel("成功次数");
    registrationLabel->setAlignment(Qt::AlignCenter);
    
    registrationLayout->addWidget(m_registrationCountValue);
    registrationLayout->addWidget(registrationLabel);
    registrationCard->setLayout(registrationLayout);
    cardsLayout->addWidget(registrationCard);
    
    mainLayout->addLayout(cardsLayout);
    
    // 快速开始指南
    QGroupBox* guideGroup = new QGroupBox("快速开始");
    QVBoxLayout* guideLayout = new QVBoxLayout();
    
    QLabel* step1 = new QLabel("1. 在\"数据管理\"页面导入源点云和目标点云");
    QLabel* step2 = new QLabel("2. 在\"设置\"页面配置ICP参数（可选）");
    QLabel* step3 = new QLabel("3. 在\"配准控制台\"页面启动配准");
    QLabel* step4 = new QLabel("4. 在\"3D可视化\"页面查看配准结果和回放");
    
    guideLayout->addWidget(step1);
    guideLayout->addWidget(step2);
    guideLayout->addWidget(step3);
    guideLayout->addWidget(step4);
    
    guideGroup->setLayout(guideLayout);
    mainLayout->addWidget(guideGroup);
    
    // 配准历史表格
    QGroupBox* historyGroup = new QGroupBox("配准历史记录");
    QVBoxLayout* historyLayout = new QVBoxLayout();
    
    m_historyTable = new ElaTableView(this);
    m_historyModel = new QStandardItemModel(this);
    m_historyModel->setHorizontalHeaderLabels({
        "时间", "源文件", "目标文件", "迭代次数", "最终RMSE", "状态"
    });
    m_historyTable->setModel(m_historyModel);
    
    historyLayout->addWidget(m_historyTable);
    historyGroup->setLayout(historyLayout);
    mainLayout->addWidget(historyGroup);
    
    mainLayout->addStretch();
}

void DashboardPage::updateCloudInfo()
{
    if (!m_registrationService) return;
    
    // 更新源点云信息
    if (m_registrationService->getSourceCloud()) {
        int points = static_cast<int>(m_registrationService->getSourceCloud()->size());
        m_sourcePointsValue->setText(QString::number(points));
    } else {
        m_sourcePointsValue->setText("0");
    }
    
    // 更新目标点云信息
    if (m_registrationService->getTargetCloud()) {
        int points = static_cast<int>(m_registrationService->getTargetCloud()->size());
        m_targetPointsValue->setText(QString::number(points));
    } else {
        m_targetPointsValue->setText("0");
    }
}

void DashboardPage::updateHistory(const QVector<RegistrationRecord>& history)
{
    m_historyModel->removeRows(0, m_historyModel->rowCount());
    
    for (const auto& record : history) {
        QList<QStandardItem*> rowItems;
        
        rowItems << new QStandardItem(record.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
        rowItems << new QStandardItem(record.sourceFile);
        rowItems << new QStandardItem(record.targetFile);
        rowItems << new QStandardItem(QString::number(record.iterations));
        rowItems << new QStandardItem(QString::number(record.finalRMSE, 'f', 6));
        rowItems << new QStandardItem(record.success ? "✓ 成功" : "✗ 失败");
        
        m_historyModel->appendRow(rowItems);
    }
    
    // 更新配准次数
    int successCount = 0;
    for (const auto& record : history) {
        if (record.success) successCount++;
    }
    m_registrationCountValue->setText(QString::number(successCount));
}
