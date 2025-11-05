#include "ui/pages/settingspage.h"

#include "services/settingsservice.h"

#include <ElaLineEdit.h>
#include <ElaText.h>
#include <ElaToggleSwitch.h>

#include <functional>
#include <QHBoxLayout>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("系统设置"));
    buildUi();
    connectControls();
}

void SettingsPage::setSettingsService(SettingsService *service)
{
    if (m_settingsService == service) {
        return;
    }
    if (m_settingsService) {
        disconnect(m_settingsService, nullptr, this, nullptr);
    }
    m_settingsService = service;
    if (!m_settingsService) {
        return;
    }

    connect(m_settingsService, &SettingsService::settingsChanged, this, &SettingsPage::applySettings);
    connect(m_settingsService, &SettingsService::incidentSettingsChanged, this, [this](const SettingsService::IncidentSettings &settings) {
        m_isApplying = true;
        if (m_incidentEndpointEdit) {
            m_incidentEndpointEdit->setText(settings.endpoint);
        }
        if (m_incidentMockSwitch) {
            m_incidentMockSwitch->setIsToggled(settings.useMock);
        }
        m_isApplying = false;
    });

    applySettings(m_settingsService->settings());
}

void SettingsPage::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(18);

    auto addSection = [this, layout](const QString &titleText) {
        auto *sectionLabel = new ElaText(titleText, this);
        sectionLabel->setTextStyle(ElaTextType::Subtitle);
        sectionLabel->setTextPixelSize(16);
        layout->addWidget(sectionLabel);
    };

    auto makeToggleRow = [this, layout](const QString &labelText, ElaToggleSwitch **store) {
        auto *row = new QHBoxLayout();
        row->setSpacing(12);
        auto *label = new ElaText(labelText, this);
        label->setTextStyle(ElaTextType::Body);
        label->setTextPixelSize(13);
        row->addWidget(label);
        row->addStretch();
        auto *toggle = new ElaToggleSwitch(this);
        row->addWidget(toggle);
        layout->addLayout(row);
        if (store) {
            *store = toggle;
        }
        return toggle;
    };

    addSection(tr("外观与主题"));
    makeToggleRow(tr("跟随系统主题"), &m_followSystemSwitch);
    makeToggleRow(tr("深色模式优先"), &m_preferDarkSwitch);
    makeToggleRow(tr("地图界面显示参考网格"), &m_showGridSwitch);

    addSection(tr("实时体验"));
    makeToggleRow(tr("启用视图平滑动画"), &m_smoothSwitch);
    makeToggleRow(tr("启动时恢复上次会话"), &m_restoreSwitch);

    addSection(tr("事件监控"));
    auto *endpointRow = new QHBoxLayout();
    endpointRow->setSpacing(12);
    auto *endpointLabel = new ElaText(tr("订阅地址"), this);
    endpointLabel->setTextStyle(ElaTextType::Body);
    endpointLabel->setTextPixelSize(13);
    endpointRow->addWidget(endpointLabel);
    m_incidentEndpointEdit = new ElaLineEdit(this);
    m_incidentEndpointEdit->setPlaceholderText(QStringLiteral("ws://localhost:4000/stream"));
    endpointRow->addWidget(m_incidentEndpointEdit, 1);
    layout->addLayout(endpointRow);

    makeToggleRow(tr("使用模拟数据"), &m_incidentMockSwitch);

    layout->addStretch(1);
}

void SettingsPage::connectControls()
{
    auto connectToggle = [this](ElaToggleSwitch *toggle, auto updater) {
        if (!toggle) {
            return;
        }
        connect(toggle, &ElaToggleSwitch::toggled, this, [this, updater, toggle](bool checked) {
            if (m_isApplying) {
                return;
            }
            if (m_settingsService) {
                std::invoke(updater, m_settingsService, checked);
            }
            if (toggle == m_followSystemSwitch || toggle == m_preferDarkSwitch) {
                updateThemeMode();
            }
        });
    };

    connectToggle(m_followSystemSwitch, &SettingsService::setFollowSystemTheme);
    connectToggle(m_preferDarkSwitch, &SettingsService::setPreferDarkTheme);
    connectToggle(m_showGridSwitch, &SettingsService::setShowMapGrid);
    connectToggle(m_smoothSwitch, &SettingsService::setSmoothAnimation);
    connectToggle(m_restoreSwitch, &SettingsService::setRestoreSession);
    connectToggle(m_incidentMockSwitch, &SettingsService::setIncidentUseMock);

    if (m_incidentEndpointEdit) {
        connect(m_incidentEndpointEdit, &QLineEdit::editingFinished, this, [this]() {
            if (!m_settingsService || m_isApplying) {
                return;
            }
            const QString text = m_incidentEndpointEdit->text().trimmed();
            m_settingsService->setIncidentEndpoint(text);
        });
    }
}

void SettingsPage::applySettings(const SettingsService::AppSettings &settings)
{
    m_isApplying = true;

    auto applyToggle = [](ElaToggleSwitch *toggle, bool value) {
        if (toggle) {
            toggle->setIsToggled(value);
        }
    };

    applyToggle(m_followSystemSwitch, settings.followSystemTheme);
    applyToggle(m_preferDarkSwitch, settings.preferDarkTheme);
    applyToggle(m_showGridSwitch, settings.showMapGrid);
    applyToggle(m_smoothSwitch, settings.smoothAnimation);
    applyToggle(m_restoreSwitch, settings.restoreSession);
    applyToggle(m_incidentMockSwitch, settings.incidentUseMock);

    if (m_incidentEndpointEdit) {
        m_incidentEndpointEdit->setText(settings.incidentEndpoint);
    }

    m_isApplying = false;
    updateThemeMode();
}

void SettingsPage::updateThemeMode()
{
    if (!m_followSystemSwitch || !m_preferDarkSwitch) {
        return;
    }

    ElaThemeType::ThemeMode mode = ElaThemeType::ThemeMode::Light;
    if (!m_followSystemSwitch->getIsToggled()) {
        mode = m_preferDarkSwitch->getIsToggled() ? ElaThemeType::ThemeMode::Dark : ElaThemeType::ThemeMode::Light;
    }

    emit themeModeChanged(mode);
}
