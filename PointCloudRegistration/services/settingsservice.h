#ifndef SETTINGSSERVICE_H
#define SETTINGSSERVICE_H

#include <QObject>
#include <QSettings>
#include <QColor>
#include "core/icpengine.h"

/**
 * @brief 应用设置
 */
struct AppSettings {
    // ICP参数
    ICPParameters icpParams;
    
    // 显示设置
    float sourcePointSize = 2.0f;
    float targetPointSize = 2.0f;
    QColor sourceColor = QColor(255, 100, 100);  // 红色
    QColor targetColor = QColor(100, 100, 255);  // 蓝色
    bool showGrid = true;
    bool smoothRendering = true;
    
    // 窗口设置
    bool followSystemTheme = true;
    bool preferDarkMode = true;
    bool restoreLastSession = true;
};

/**
 * @brief 设置服务
 */
class SettingsService : public QObject
{
    Q_OBJECT

public:
    explicit SettingsService(QObject *parent = nullptr);
    ~SettingsService() override;
    
    // 加载和保存
    void load();
    void save();
    
    // 获取和设置
    AppSettings getSettings() const { return m_settings; }
    void setSettings(const AppSettings& settings);
    
    // 便捷访问
    ICPParameters getICPParameters() const { return m_settings.icpParams; }
    void setICPParameters(const ICPParameters& params);
    
signals:
    void settingsChanged(const AppSettings& settings);
    void icpParametersChanged(const ICPParameters& params);
    
private:
    AppSettings m_settings;
    QSettings* m_qsettings;
};

#endif // SETTINGSSERVICE_H
