#include "settingspage.h"
#include "ElaSpinBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaPushButton.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
    , m_settingsService(nullptr)
    , m_isLoading(false)
{
    buildUI();
}

void SettingsPage::setSettingsService(SettingsService* service)
{
    m_settingsService = service;
    if (m_settingsService) {
        loadSettings();
    }
}

void SettingsPage::buildUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // 标题
    ElaText* titleText = new ElaText("配准参数设置", this);
    titleText->setTextPixelSize(24);
    mainLayout->addWidget(titleText);
    
    // 创建滚动区域
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(20);
    
    // ICP参数组
    QGroupBox* icpGroup = new QGroupBox("ICP算法参数");
    QFormLayout* icpLayout = new QFormLayout();
    
    m_maxIterationsSpinBox = new ElaSpinBox(this);
    m_maxIterationsSpinBox->setRange(1, 1000);
    m_maxIterationsSpinBox->setValue(50);
    icpLayout->addRow("最大迭代次数:", m_maxIterationsSpinBox);
    
    m_toleranceSpinBox = new ElaDoubleSpinBox(this);
    m_toleranceSpinBox->setDecimals(10);
    m_toleranceSpinBox->setRange(1e-10, 1e-2);
    m_toleranceSpinBox->setValue(1e-6);
    m_toleranceSpinBox->setSingleStep(1e-7);
    icpLayout->addRow("收敛容差:", m_toleranceSpinBox);
    
    m_sigmaMultiplierSpinBox = new ElaDoubleSpinBox(this);
    m_sigmaMultiplierSpinBox->setRange(1.0, 5.0);
    m_sigmaMultiplierSpinBox->setValue(3.0);
    m_sigmaMultiplierSpinBox->setSingleStep(0.1);
    icpLayout->addRow("Sigma阈值倍数:", m_sigmaMultiplierSpinBox);
    
    m_octreeMaxPointsSpinBox = new ElaSpinBox(this);
    m_octreeMaxPointsSpinBox->setRange(5, 100);
    m_octreeMaxPointsSpinBox->setValue(10);
    icpLayout->addRow("八叉树节点最大点数:", m_octreeMaxPointsSpinBox);
    
    m_octreeMaxDepthSpinBox = new ElaSpinBox(this);
    m_octreeMaxDepthSpinBox->setRange(10, 50);
    m_octreeMaxDepthSpinBox->setValue(20);
    icpLayout->addRow("八叉树最大深度:", m_octreeMaxDepthSpinBox);
    
    icpGroup->setLayout(icpLayout);
    scrollLayout->addWidget(icpGroup);
    
    // 显示设置组
    QGroupBox* displayGroup = new QGroupBox("显示设置");
    QFormLayout* displayLayout = new QFormLayout();
    
    m_sourcePointSizeSpinBox = new ElaDoubleSpinBox(this);
    m_sourcePointSizeSpinBox->setRange(0.5, 10.0);
    m_sourcePointSizeSpinBox->setValue(2.0);
    m_sourcePointSizeSpinBox->setSingleStep(0.5);
    displayLayout->addRow("源点云点大小:", m_sourcePointSizeSpinBox);
    
    m_targetPointSizeSpinBox = new ElaDoubleSpinBox(this);
    m_targetPointSizeSpinBox->setRange(0.5, 10.0);
    m_targetPointSizeSpinBox->setValue(2.0);
    m_targetPointSizeSpinBox->setSingleStep(0.5);
    displayLayout->addRow("目标点云点大小:", m_targetPointSizeSpinBox);
    
    m_showGridSwitch = new ElaToggleSwitch(this);
    m_showGridSwitch->setIsToggled(true);
    displayLayout->addRow("显示网格:", m_showGridSwitch);
    
    m_smoothRenderingSwitch = new ElaToggleSwitch(this);
    m_smoothRenderingSwitch->setIsToggled(true);
    displayLayout->addRow("平滑渲染:", m_smoothRenderingSwitch);
    
    displayGroup->setLayout(displayLayout);
    scrollLayout->addWidget(displayGroup);
    
    // 窗口设置组
    QGroupBox* windowGroup = new QGroupBox("窗口设置");
    QFormLayout* windowLayout = new QFormLayout();
    
    m_followSystemThemeSwitch = new ElaToggleSwitch(this);
    m_followSystemThemeSwitch->setIsToggled(true);
    windowLayout->addRow("跟随系统主题:", m_followSystemThemeSwitch);
    
    m_preferDarkModeSwitch = new ElaToggleSwitch(this);
    m_preferDarkModeSwitch->setIsToggled(true);
    windowLayout->addRow("偏好深色模式:", m_preferDarkModeSwitch);
    
    m_restoreSessionSwitch = new ElaToggleSwitch(this);
    m_restoreSessionSwitch->setIsToggled(true);
    windowLayout->addRow("恢复上次会话:", m_restoreSessionSwitch);
    
    windowGroup->setLayout(windowLayout);
    scrollLayout->addWidget(windowGroup);
    
    scrollLayout->addStretch();
    scrollContent->setLayout(scrollLayout);
    scrollArea->setWidget(scrollContent);
    
    mainLayout->addWidget(scrollArea);
    
    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    ElaPushButton* applyButton = new ElaPushButton("应用设置", this);
    ElaPushButton* resetButton = new ElaPushButton("恢复默认", this);
    
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(resetButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(applyButton, &ElaPushButton::clicked, this, &SettingsPage::onApplySettings);
    connect(resetButton, &ElaPushButton::clicked, this, &SettingsPage::onResetSettings);
}

void SettingsPage::loadSettings()
{
    if (!m_settingsService) return;
    
    m_isLoading = true;
    applySettings(m_settingsService->getSettings());
    m_isLoading = false;
}

void SettingsPage::applySettings(const AppSettings& settings)
{
    m_maxIterationsSpinBox->setValue(settings.icpParams.maxIterations);
    m_toleranceSpinBox->setValue(settings.icpParams.tolerance);
    m_sigmaMultiplierSpinBox->setValue(settings.icpParams.sigmaMultiplier);
    m_octreeMaxPointsSpinBox->setValue(settings.icpParams.octreeMaxPoints);
    m_octreeMaxDepthSpinBox->setValue(settings.icpParams.octreeMaxDepth);
    
    m_sourcePointSizeSpinBox->setValue(settings.sourcePointSize);
    m_targetPointSizeSpinBox->setValue(settings.targetPointSize);
    m_showGridSwitch->setIsToggled(settings.showGrid);
    m_smoothRenderingSwitch->setIsToggled(settings.smoothRendering);
    
    m_followSystemThemeSwitch->setIsToggled(settings.followSystemTheme);
    m_preferDarkModeSwitch->setIsToggled(settings.preferDarkMode);
    m_restoreSessionSwitch->setIsToggled(settings.restoreLastSession);
}

void SettingsPage::onApplySettings()
{
    if (!m_settingsService || m_isLoading) return;
    
    AppSettings settings;
    
    // ICP参数
    settings.icpParams.maxIterations = m_maxIterationsSpinBox->value();
    settings.icpParams.tolerance = m_toleranceSpinBox->value();
    settings.icpParams.sigmaMultiplier = m_sigmaMultiplierSpinBox->value();
    settings.icpParams.octreeMaxPoints = m_octreeMaxPointsSpinBox->value();
    settings.icpParams.octreeMaxDepth = m_octreeMaxDepthSpinBox->value();
    
    // 显示设置
    settings.sourcePointSize = static_cast<float>(m_sourcePointSizeSpinBox->value());
    settings.targetPointSize = static_cast<float>(m_targetPointSizeSpinBox->value());
    settings.showGrid = m_showGridSwitch->getIsToggled();
    settings.smoothRendering = m_smoothRenderingSwitch->getIsToggled();
    
    // 窗口设置
    settings.followSystemTheme = m_followSystemThemeSwitch->getIsToggled();
    settings.preferDarkMode = m_preferDarkModeSwitch->getIsToggled();
    settings.restoreLastSession = m_restoreSessionSwitch->getIsToggled();
    
    m_settingsService->setSettings(settings);
    emit settingsChanged();
}

void SettingsPage::onResetSettings()
{
    AppSettings defaultSettings;
    applySettings(defaultSettings);
}
