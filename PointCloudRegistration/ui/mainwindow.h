#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ElaWindow.h"
#include "services/registrationservice.h"
#include "services/settingsservice.h"

class DashboardPage;
class DataManagerPage;
class RegistrationPage;
class VisualizationPage;
class SettingsPage;

/**
 * @brief 主窗口
 * 
 * 集成所有页面和导航
 */
class MainWindow : public ElaWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initializeWindow();
    void setupServices();
    void setupPages();
    void setupNavigation();
    void connectSignals();
    void restoreSession();
    void saveSession();
    
    // 页面
    DashboardPage* m_dashboardPage;
    DataManagerPage* m_dataManagerPage;
    RegistrationPage* m_registrationPage;
    VisualizationPage* m_visualizationPage;
    SettingsPage* m_settingsPage;
    
    // 页面键
    QString m_dashboardKey;
    QString m_dataManagerKey;
    QString m_registrationKey;
    QString m_visualizationKey;
    QString m_settingsKey;
    
    // 服务
    RegistrationService* m_registrationService;
    SettingsService* m_settingsService;
};

#endif // MAINWINDOW_H
