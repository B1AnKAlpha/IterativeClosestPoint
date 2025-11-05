#include "visualizationpage.h"
#include "ElaPushButton.h"
#include "ElaSlider.h"
#include "ElaText.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

VisualizationPage::VisualizationPage(QWidget *parent)
    : QWidget(parent)
    , m_registrationService(nullptr)
{
    buildUI();
}

void VisualizationPage::setRegistrationService(RegistrationService* service)
{
    m_registrationService = service;
}

void VisualizationPage::buildUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 标题
    ElaText* titleText = new ElaText("3D可视化", this);
    titleText->setTextPixelSize(20);
    mainLayout->addWidget(titleText);
    
    // 3D查看器
    m_viewer = new PointCloudViewer(this);
    m_viewer->setMinimumHeight(400);
    mainLayout->addWidget(m_viewer, 1);
    
    connect(m_viewer, &PointCloudViewer::iterationChanged, 
            this, &VisualizationPage::onIterationChanged);
    
    // 播放控制组
    QGroupBox* controlGroup = new QGroupBox("迭代回放控制");
    QVBoxLayout* controlLayout = new QVBoxLayout();
    
    // 按钮行
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_firstButton = new ElaPushButton("上一迭代", this);
    m_lastButton = new ElaPushButton("下一迭代", this);
    
    m_firstButton->setMaximumWidth(120);
    m_lastButton->setMaximumWidth(120);
    
    buttonLayout->addWidget(m_firstButton);
    buttonLayout->addWidget(m_lastButton);
    buttonLayout->addStretch();
    
    controlLayout->addLayout(buttonLayout);
    
    // 迭代滑块
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    QLabel* iterLabel = new QLabel("迭代:");
    m_iterationSlider = new ElaSlider(Qt::Horizontal, this);
    m_iterationSlider->setRange(-1, 0);
    m_iterationSlider->setValue(-1);
    
    sliderLayout->addWidget(iterLabel);
    sliderLayout->addWidget(m_iterationSlider, 1);
    controlLayout->addLayout(sliderLayout);
    
    controlGroup->setLayout(controlLayout);
    mainLayout->addWidget(controlGroup);
    
    // 信息显示组
    QGroupBox* infoGroup = new QGroupBox("当前迭代信息");
    QVBoxLayout* infoLayout = new QVBoxLayout();
    
    m_iterationLabel = new QLabel("迭代: 初始状态");
    m_rmseLabel = new QLabel("RMSE: -");
    m_transformLabel = new QLabel("变换: 无");
    
    infoLayout->addWidget(m_iterationLabel);
    infoLayout->addWidget(m_rmseLabel);
    infoLayout->addWidget(m_transformLabel);
    
    infoGroup->setLayout(infoLayout);
    mainLayout->addWidget(infoGroup);
    
    // 连接信号
    connect(m_firstButton, &ElaPushButton::clicked, this, &VisualizationPage::onFirstFrame);
    connect(m_lastButton, &ElaPushButton::clicked, this, &VisualizationPage::onLastFrame);
    connect(m_iterationSlider, &ElaSlider::valueChanged, this, &VisualizationPage::onSliderChanged);
    
    // 初始禁用控制
    updatePlaybackControls();
}

void VisualizationPage::updateViewer()
{
    if (!m_registrationService) return;
    
    // 使用原始源点云而不是变换后的源点云
    const PointCloud* originalSource = m_registrationService->getOriginalSourceCloud();
    if (originalSource) {
        // 创建副本,因为viewer需要可修改的指针
        PointCloud* sourceCopy = new PointCloud();
        sourceCopy->points = originalSource->points;
        sourceCopy->color = originalSource->color;
        sourceCopy->computeBounds();
        m_viewer->setSourceCloud(sourceCopy);
    }
    
    m_viewer->setTargetCloud(const_cast<PointCloud*>(m_registrationService->getTargetCloud()));
}

void VisualizationPage::loadIterationHistory(const std::vector<IterationResult>& history)
{
    m_viewer->setIterationHistory(history);
    m_iterationSlider->setRange(-1, static_cast<int>(history.size()) - 1);
    m_iterationSlider->setValue(-1);
    updatePlaybackControls();
    updateIterationInfo();
}

void VisualizationPage::onFirstFrame()
{
    // 上一迭代
    int current = m_viewer->getCurrentIteration();
    if (current > -1) {
        int prev = current - 1;
        m_viewer->setCurrentIteration(prev);
        m_iterationSlider->setValue(prev);
    }
}

void VisualizationPage::onLastFrame()
{
    // 下一迭代
    int current = m_viewer->getCurrentIteration();
    int max = m_viewer->getIterationCount() - 1;
    if (current < max) {
        int next = current + 1;
        m_viewer->setCurrentIteration(next);
        m_iterationSlider->setValue(next);
    }
}

void VisualizationPage::onSliderChanged(int value)
{
    m_viewer->setCurrentIteration(value);
}

void VisualizationPage::onIterationChanged(int iteration)
{
    updateIterationInfo();
}

void VisualizationPage::updatePlaybackControls()
{
    bool hasHistory = m_viewer->getIterationCount() > 0;
    
    m_firstButton->setEnabled(hasHistory);
    m_lastButton->setEnabled(hasHistory);
    m_iterationSlider->setEnabled(hasHistory);
}

void VisualizationPage::updateIterationInfo()
{
    int current = m_viewer->getCurrentIteration();
    int total = m_viewer->getIterationCount();
    
    if (current < 0) {
        m_iterationLabel->setText("迭代: 初始状态");
        m_rmseLabel->setText("RMSE: -");
        m_transformLabel->setText("变换: 无");
    } else if (current < total) {
        // 这里需要从ICPEngine获取迭代详情
        // 简化版本只显示基本信息
        m_iterationLabel->setText(QString("迭代: %1 / %2").arg(current + 1).arg(total));
        m_rmseLabel->setText("RMSE: 查看配准页面");
        m_transformLabel->setText("变换: 已应用累积变换");
    }
}
