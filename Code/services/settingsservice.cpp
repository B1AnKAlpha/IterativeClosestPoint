#include "services/settingsservice.h"

#include <QCoreApplication>
#include <QSettings>

namespace {
constexpr char kOrgName[] = "WidgetQuadtree";
constexpr char kAppName[] = "WidgetSuite";
}

SettingsService::SettingsService(QObject *parent)
    : QObject(parent)
{
    m_qsettings = new QSettings(QString::fromLatin1(kOrgName), QString::fromLatin1(kAppName), this);
    load();
}

SettingsService::AppSettings SettingsService::settings() const
{
    return m_settings;
}

SettingsService::IncidentSettings SettingsService::incidentSettings() const
{
    IncidentSettings incident;
    incident.endpoint = m_settings.incidentEndpoint;
    incident.useMock = m_settings.incidentUseMock;
    return incident;
}

void SettingsService::setFollowSystemTheme(bool enabled)
{
    if (m_settings.followSystemTheme == enabled) {
        return;
    }
    m_settings.followSystemTheme = enabled;
    save();
    emit settingsChanged(m_settings);
}

void SettingsService::setPreferDarkTheme(bool enabled)
{
    if (m_settings.preferDarkTheme == enabled) {
        return;
    }
    m_settings.preferDarkTheme = enabled;
    save();
    emit settingsChanged(m_settings);
}

void SettingsService::setShowMapGrid(bool enabled)
{
    if (m_settings.showMapGrid == enabled) {
        return;
    }
    m_settings.showMapGrid = enabled;
    save();
    emit settingsChanged(m_settings);
}

void SettingsService::setSmoothAnimation(bool enabled)
{
    if (m_settings.smoothAnimation == enabled) {
        return;
    }
    m_settings.smoothAnimation = enabled;
    save();
    emit settingsChanged(m_settings);
}

void SettingsService::setRestoreSession(bool enabled)
{
    if (m_settings.restoreSession == enabled) {
        return;
    }
    m_settings.restoreSession = enabled;
    save();
    emit settingsChanged(m_settings);
}

void SettingsService::setLastPageKey(const QString &pageKey)
{
    if (m_settings.lastPageKey == pageKey) {
        return;
    }
    m_settings.lastPageKey = pageKey;
    save();
}

void SettingsService::setIncidentEndpoint(const QString &endpoint)
{
    if (m_settings.incidentEndpoint == endpoint) {
        return;
    }
    m_settings.incidentEndpoint = endpoint;
    save();
    emit incidentSettingsChanged(incidentSettings());
}

void SettingsService::setIncidentUseMock(bool useMock)
{
    if (m_settings.incidentUseMock == useMock) {
        return;
    }
    m_settings.incidentUseMock = useMock;
    save();
    emit incidentSettingsChanged(incidentSettings());
}

void SettingsService::load()
{
    m_settings.followSystemTheme = m_qsettings->value(QStringLiteral("appearance/followSystem"), true).toBool();
    m_settings.preferDarkTheme = m_qsettings->value(QStringLiteral("appearance/preferDark"), false).toBool();
    m_settings.showMapGrid = m_qsettings->value(QStringLiteral("view/showGrid"), true).toBool();
    m_settings.smoothAnimation = m_qsettings->value(QStringLiteral("view/smoothAnimation"), true).toBool();
    m_settings.restoreSession = m_qsettings->value(QStringLiteral("general/restoreSession"), true).toBool();
    m_settings.lastPageKey = m_qsettings->value(QStringLiteral("general/lastPageKey"), QString()).toString();
    m_settings.incidentEndpoint = m_qsettings->value(QStringLiteral("incident/endpoint"), QStringLiteral("ws://localhost:4000/stream")).toString();
    m_settings.incidentUseMock = m_qsettings->value(QStringLiteral("incident/useMock"), true).toBool();

    emitAllSignals();
}

void SettingsService::save() const
{
    m_qsettings->setValue(QStringLiteral("appearance/followSystem"), m_settings.followSystemTheme);
    m_qsettings->setValue(QStringLiteral("appearance/preferDark"), m_settings.preferDarkTheme);
    m_qsettings->setValue(QStringLiteral("view/showGrid"), m_settings.showMapGrid);
    m_qsettings->setValue(QStringLiteral("view/smoothAnimation"), m_settings.smoothAnimation);
    m_qsettings->setValue(QStringLiteral("general/restoreSession"), m_settings.restoreSession);
    m_qsettings->setValue(QStringLiteral("general/lastPageKey"), m_settings.lastPageKey);
    m_qsettings->setValue(QStringLiteral("incident/endpoint"), m_settings.incidentEndpoint);
    m_qsettings->setValue(QStringLiteral("incident/useMock"), m_settings.incidentUseMock);
}

void SettingsService::emitAllSignals()
{
    emit settingsChanged(m_settings);
    emit incidentSettingsChanged(incidentSettings());
}
