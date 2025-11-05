#include "datamanagerpage.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

DataManagerPage::DataManagerPage(QWidget *parent)
    : QWidget(parent)
    , m_registrationService(nullptr)
{
    buildUI();
}

void DataManagerPage::setRegistrationService(RegistrationService* service)
{
    m_registrationService = service;
    
    if (m_registrationService) {
        connect(m_registrationService, &RegistrationService::sourceCloudLoaded,
                this, &DataManagerPage::onSourceCloudLoaded);
        connect(m_registrationService, &RegistrationService::targetCloudLoaded,
                this, &DataManagerPage::onTargetCloudLoaded);
        connect(m_registrationService, &RegistrationService::cloudLoadError,
                this, &DataManagerPage::onCloudLoadError);
    }
}

void DataManagerPage::buildUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);
    
    // 标题
    ElaText* titleText = new ElaText("数据管理", this);
    titleText->setTextPixelSize(24);
    mainLayout->addWidget(titleText);
    
    // 源点云组
    QGroupBox* sourceGroup = new QGroupBox("源点云（待配准）");
    QVBoxLayout* sourceLayout = new QVBoxLayout();
    
    m_sourceFileLabel = new QLabel("文件: 未加载");
    m_sourcePointsLabel = new QLabel("点数: -");
    m_sourceBoundsLabel = new QLabel("边界: -");
    
    sourceLayout->addWidget(m_sourceFileLabel);
    sourceLayout->addWidget(m_sourcePointsLabel);
    sourceLayout->addWidget(m_sourceBoundsLabel);
    
    QHBoxLayout* sourceButtonLayout = new QHBoxLayout();
    m_loadSourceButton = new ElaPushButton("导入源点云", this);
    m_clearSourceButton = new ElaPushButton("清除", this);
    m_clearSourceButton->setEnabled(false);
    
    sourceButtonLayout->addWidget(m_loadSourceButton);
    sourceButtonLayout->addWidget(m_clearSourceButton);
    sourceButtonLayout->addStretch();
    
    sourceLayout->addLayout(sourceButtonLayout);
    sourceGroup->setLayout(sourceLayout);
    mainLayout->addWidget(sourceGroup);
    
    // 目标点云组
    QGroupBox* targetGroup = new QGroupBox("目标点云（参考）");
    QVBoxLayout* targetLayout = new QVBoxLayout();
    
    m_targetFileLabel = new QLabel("文件: 未加载");
    m_targetPointsLabel = new QLabel("点数: -");
    m_targetBoundsLabel = new QLabel("边界: -");
    
    targetLayout->addWidget(m_targetFileLabel);
    targetLayout->addWidget(m_targetPointsLabel);
    targetLayout->addWidget(m_targetBoundsLabel);
    
    QHBoxLayout* targetButtonLayout = new QHBoxLayout();
    m_loadTargetButton = new ElaPushButton("导入目标点云", this);
    m_clearTargetButton = new ElaPushButton("清除", this);
    m_clearTargetButton->setEnabled(false);
    
    targetButtonLayout->addWidget(m_loadTargetButton);
    targetButtonLayout->addWidget(m_clearTargetButton);
    targetButtonLayout->addStretch();
    
    targetLayout->addLayout(targetButtonLayout);
    targetGroup->setLayout(targetLayout);
    mainLayout->addWidget(targetGroup);
    
    // 结果保存组
    QGroupBox* resultGroup = new QGroupBox("配准结果");
    QVBoxLayout* resultLayout = new QVBoxLayout();
    
    QLabel* resultLabel = new QLabel("配准完成后，可以将配准后的源点云保存为LAS文件");
    resultLabel->setWordWrap(true);
    resultLayout->addWidget(resultLabel);
    
    m_saveResultButton = new ElaPushButton("保存配准结果", this);
    m_saveResultButton->setEnabled(false);
    resultLayout->addWidget(m_saveResultButton);
    
    resultGroup->setLayout(resultLayout);
    mainLayout->addWidget(resultGroup);
    
    mainLayout->addStretch();
    
    // 连接信号
    connect(m_loadSourceButton, &ElaPushButton::clicked, this, &DataManagerPage::onLoadSource);
    connect(m_loadTargetButton, &ElaPushButton::clicked, this, &DataManagerPage::onLoadTarget);
    connect(m_saveResultButton, &ElaPushButton::clicked, this, &DataManagerPage::onSaveResult);
    connect(m_clearSourceButton, &ElaPushButton::clicked, this, &DataManagerPage::onClearSource);
    connect(m_clearTargetButton, &ElaPushButton::clicked, this, &DataManagerPage::onClearTarget);
}

void DataManagerPage::onLoadSource()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "选择源点云文件",
        "",
        "LAS Files (*.las);;All Files (*.*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    if (m_registrationService) {
        m_loadSourceButton->setEnabled(false);
        m_sourceFileLabel->setText("正在加载...");
        m_registrationService->loadSourceCloud(filename);
    }
}

void DataManagerPage::onLoadTarget()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "选择目标点云文件",
        "",
        "LAS Files (*.las);;All Files (*.*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    if (m_registrationService) {
        m_loadTargetButton->setEnabled(false);
        m_targetFileLabel->setText("正在加载...");
        m_registrationService->loadTargetCloud(filename);
    }
}

void DataManagerPage::onSaveResult()
{
    if (!m_registrationService || !m_registrationService->getSourceCloud()) {
        QMessageBox::warning(this, "警告", "没有可保存的配准结果");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(
        this,
        "保存配准结果",
        "registered_result.las",
        "LAS Files (*.las);;All Files (*.*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    if (m_registrationService->saveRegisteredCloud(filename)) {
        QMessageBox::information(this, "成功", "配准结果已保存");
    } else {
        QMessageBox::critical(this, "错误", "保存配准结果失败");
    }
}

void DataManagerPage::onClearSource()
{
    if (m_registrationService) {
        m_registrationService->clearSourceCloud();
        m_sourceFileLabel->setText("文件: 未加载");
        m_sourcePointsLabel->setText("点数: -");
        m_sourceBoundsLabel->setText("边界: -");
        m_clearSourceButton->setEnabled(false);
        m_saveResultButton->setEnabled(false);
    }
}

void DataManagerPage::onClearTarget()
{
    if (m_registrationService) {
        m_registrationService->clearTargetCloud();
        m_targetFileLabel->setText("文件: 未加载");
        m_targetPointsLabel->setText("点数: -");
        m_targetBoundsLabel->setText("边界: -");
        m_clearTargetButton->setEnabled(false);
    }
}

void DataManagerPage::onSourceCloudLoaded(const QString& filename, int pointCount)
{
    m_sourceFileLabel->setText(QString("文件: %1").arg(QFileInfo(filename).fileName()));
    m_sourcePointsLabel->setText(QString("点数: %1").arg(pointCount));
    
    if (m_registrationService && m_registrationService->getSourceCloud()) {
        auto cloud = m_registrationService->getSourceCloud();
        const_cast<PointCloud*>(cloud)->computeBounds();
        m_sourceBoundsLabel->setText(QString("边界: [%1, %2, %3] - [%4, %5, %6]")
            .arg(cloud->minX, 0, 'f', 2).arg(cloud->minY, 0, 'f', 2).arg(cloud->minZ, 0, 'f', 2)
            .arg(cloud->maxX, 0, 'f', 2).arg(cloud->maxY, 0, 'f', 2).arg(cloud->maxZ, 0, 'f', 2));
    }
    
    m_loadSourceButton->setEnabled(true);
    m_clearSourceButton->setEnabled(true);
    m_saveResultButton->setEnabled(true);
    emit fileLoaded();
}

void DataManagerPage::onTargetCloudLoaded(const QString& filename, int pointCount)
{
    m_targetFileLabel->setText(QString("文件: %1").arg(QFileInfo(filename).fileName()));
    m_targetPointsLabel->setText(QString("点数: %1").arg(pointCount));
    
    if (m_registrationService && m_registrationService->getTargetCloud()) {
        auto cloud = m_registrationService->getTargetCloud();
        const_cast<PointCloud*>(cloud)->computeBounds();
        m_targetBoundsLabel->setText(QString("边界: [%1, %2, %3] - [%4, %5, %6]")
            .arg(cloud->minX, 0, 'f', 2).arg(cloud->minY, 0, 'f', 2).arg(cloud->minZ, 0, 'f', 2)
            .arg(cloud->maxX, 0, 'f', 2).arg(cloud->maxY, 0, 'f', 2).arg(cloud->maxZ, 0, 'f', 2));
    }
    
    m_loadTargetButton->setEnabled(true);
    m_clearTargetButton->setEnabled(true);
    emit fileLoaded();
}

void DataManagerPage::onCloudLoadError(const QString& message)
{
    QMessageBox::critical(this, "错误", message);
}
