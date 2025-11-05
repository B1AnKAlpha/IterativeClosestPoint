#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include "services/settingsservice.h"

class ElaToggleSwitch;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaColorPickerButton;
class ElaText;
class QVBoxLayout;
class QFormLayout;

/**
 * @brief 设置页面
 */
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    
    void setSettingsService(SettingsService* service);
    
signals:
    void settingsChanged();
    
private slots:
    void onApplySettings();
    void onResetSettings();
    
private:
    void buildUI();
    void loadSettings();
    void applySettings(const AppSettings& settings);
    
    SettingsService* m_settingsService;
    bool m_isLoading;
    
    // ICP参数控件
    ElaSpinBox* m_maxIterationsSpinBox;
    ElaDoubleSpinBox* m_toleranceSpinBox;
    ElaDoubleSpinBox* m_sigmaMultiplierSpinBox;
    ElaSpinBox* m_octreeMaxPointsSpinBox;
    ElaSpinBox* m_octreeMaxDepthSpinBox;
    
    // 显示设置控件
    ElaDoubleSpinBox* m_sourcePointSizeSpinBox;
    ElaDoubleSpinBox* m_targetPointSizeSpinBox;
    ElaToggleSwitch* m_showGridSwitch;
    ElaToggleSwitch* m_smoothRenderingSwitch;
    
    // 窗口设置控件
    ElaToggleSwitch* m_followSystemThemeSwitch;
    ElaToggleSwitch* m_preferDarkModeSwitch;
    ElaToggleSwitch* m_restoreSessionSwitch;
};

#endif // SETTINGSPAGE_H
