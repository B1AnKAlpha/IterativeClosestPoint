#include "mainwindow.h"
#include "pages/dashboardpage.h"
#include "pages/datamanagerpage.h"
#include "pages/registrationpage.h"
#include "pages/visualizationpage.h"
#include "pages/settingspage.h"
#include "ElaTheme.h"
#include <QCloseEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : ElaWindow(parent)
    , m_dashboardPage(nullptr)
    , m_dataManagerPage(nullptr)
    , m_registrationPage(nullptr)
    , m_visualizationPage(nullptr)
    , m_settingsPage(nullptr)
    , m_registrationService(nullptr)
    , m_settingsService(nullptr)
{
    initializeWindow();
    setupServices();
    setupPages();
    setupNavigation();
    connectSignals();
    restoreSession();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initializeWindow()
{
    setWindowTitle("点云配准系统");
    setWindowIcon(QIcon(":/icon.png"));
    resize(1400, 900);
    
    // 设置主题
    setIsNavigationBarEnable(true);
    setUserInfoCardVisible(false);
    setWindowButtonFlags(ElaAppBarType::MinimizeButtonHint | 
                         ElaAppBarType::MaximizeButtonHint | 
                         ElaAppBarType::CloseButtonHint);
}

void MainWindow::setupServices()
{
    // 创建服务
    m_settingsService = new SettingsService(this);
    m_registrationService = new RegistrationService(this);
    
    // 应用主题设置
    AppSettings settings = m_settingsService->getSettings();
    if (settings.preferDarkMode) {
        ElaTheme::getInstance()->setThemeMode(ElaThemeType::Dark);
    } else {
        ElaTheme::getInstance()->setThemeMode(ElaThemeType::Light);
    }
}

void MainWindow::setupPages()
{
    // 创建Dashboard页面
    m_dashboardPage = new DashboardPage(this);
    m_dashboardPage->setRegistrationService(m_registrationService);
    if (addPageNode("概览", m_dashboardPage, ElaIconType::House) == ElaNavigationType::Success) {
        m_dashboardKey = "概览";
    }
    
    // 创建Data Manager页面
    m_dataManagerPage = new DataManagerPage(this);
    m_dataManagerPage->setRegistrationService(m_registrationService);
    if (addPageNode("数据管理", m_dataManagerPage, ElaIconType::FolderOpen) == ElaNavigationType::Success) {
        m_dataManagerKey = "数据管理";
    }
    
    // 创建Registration页面
    m_registrationPage = new RegistrationPage(this);
    m_registrationPage->setRegistrationService(m_registrationService);
    m_registrationPage->setSettingsService(m_settingsService);
    if (addPageNode("配准控制台", m_registrationPage, ElaIconType::Gears) == ElaNavigationType::Success) {
        m_registrationKey = "配准控制台";
    }
    
    // 创建Visualization页面
    m_visualizationPage = new VisualizationPage(this);
    m_visualizationPage->setRegistrationService(m_registrationService);
    if (addPageNode("3D可视化", m_visualizationPage, ElaIconType::Cube) == ElaNavigationType::Success) {
        m_visualizationKey = "3D可视化";
    }
    
    // 创建Settings页面
    m_settingsPage = new SettingsPage(this);
    m_settingsPage->setSettingsService(m_settingsService);
    if (addPageNode("设置", m_settingsPage, ElaIconType::Wrench) == ElaNavigationType::Success) {
        m_settingsKey = "设置";
    }
}

void MainWindow::setupNavigation()
{
    // 设置导航栏标题
    setNavigationBarDisplayMode(ElaNavigationType::Maximal);
}

void MainWindow::connectSignals()
{
    // 连接数据加载信号到可视化更新
    connect(m_dataManagerPage, &DataManagerPage::fileLoaded, [this]() {
        m_visualizationPage->updateViewer();
    });
    
    // 连接配准完成信号
    connect(m_registrationService, &RegistrationService::registrationFinished, 
            [this](bool success, const QString& message) {
        if (success) {
            // 更新可视化页面
            m_visualizationPage->updateViewer();
            // 加载迭代历史
            m_visualizationPage->loadIterationHistory(m_registrationService->getIterationHistory());
        }
    });
    
    // 连接设置变化信号
    connect(m_settingsService, &SettingsService::settingsChanged, 
            [this](const AppSettings& settings) {
        // 更新3D查看器设置
        auto viewer = m_visualizationPage->getViewer();
        viewer->setShowGrid(settings.showGrid);
        viewer->setSmoothRendering(settings.smoothRendering);
        viewer->setPointSize(settings.sourcePointSize);
        viewer->setSourceColor(settings.sourceColor);
        viewer->setTargetColor(settings.targetColor);
        
        // 更新主题
        if (settings.preferDarkMode) {
            ElaTheme::getInstance()->setThemeMode(ElaThemeType::Dark);
        } else {
            ElaTheme::getInstance()->setThemeMode(ElaThemeType::Light);
        }
    });
}

void MainWindow::restoreSession()
{
    AppSettings settings = m_settingsService->getSettings();
    if (settings.restoreLastSession) {
        // 恢复上次会话的页面
        // 这里可以添加恢复逻辑
    }
}

void MainWindow::saveSession()
{
    // 保存当前会话状态
    m_settingsService->save();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 检查是否有正在进行的配准
    if (m_registrationService && m_registrationService->isRegistering()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认退出",
            "配准正在进行中，确定要退出吗？",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
        
        m_registrationService->stopRegistration();
    }
    
    saveSession();
    event->accept();
}
