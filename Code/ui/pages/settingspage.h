#ifndef UI_PAGES_SETTINGSPAGE_H
#define UI_PAGES_SETTINGSPAGE_H

#include <QWidget>

#include "ElaTheme.h"
#include "services/settingsservice.h"

class ElaToggleSwitch;
class QLineEdit;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void setSettingsService(SettingsService *service);

signals:
    void themeModeChanged(ElaThemeType::ThemeMode type);

private:
    void buildUi();
    void applySettings(const SettingsService::AppSettings &settings);
    void connectControls();
    void updateThemeMode();

    bool m_isApplying{false};

    SettingsService *m_settingsService{nullptr};
    ElaToggleSwitch *m_followSystemSwitch{nullptr};
    ElaToggleSwitch *m_preferDarkSwitch{nullptr};
    ElaToggleSwitch *m_showGridSwitch{nullptr};
    ElaToggleSwitch *m_smoothSwitch{nullptr};
    ElaToggleSwitch *m_restoreSwitch{nullptr};
    ElaToggleSwitch *m_incidentMockSwitch{nullptr};
    QLineEdit *m_incidentEndpointEdit{nullptr};
};

#endif // UI_PAGES_SETTINGSPAGE_H
