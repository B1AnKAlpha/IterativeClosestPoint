#ifndef SERVICES_SETTINGSSERVICE_H
#define SERVICES_SETTINGSSERVICE_H

#include <QObject>
#include <QString>

class QSettings;

class SettingsService : public QObject
{
    Q_OBJECT

public:
    struct AppSettings {
        bool followSystemTheme{true};
        bool preferDarkTheme{false};
        bool showMapGrid{true};
        bool smoothAnimation{true};
        bool restoreSession{true};
        QString incidentEndpoint{QStringLiteral("ws://localhost:4000/stream")};
        bool incidentUseMock{true};
        QString lastPageKey;
    };

    struct IncidentSettings {
        QString endpoint;
        bool useMock{true};
    };

    explicit SettingsService(QObject *parent = nullptr);

    AppSettings settings() const;
    IncidentSettings incidentSettings() const;

public slots:
    void setFollowSystemTheme(bool enabled);
    void setPreferDarkTheme(bool enabled);
    void setShowMapGrid(bool enabled);
    void setSmoothAnimation(bool enabled);
    void setRestoreSession(bool enabled);
    void setLastPageKey(const QString &pageKey);
    void setIncidentEndpoint(const QString &endpoint);
    void setIncidentUseMock(bool useMock);

signals:
    void settingsChanged(const SettingsService::AppSettings &settings);
    void incidentSettingsChanged(const SettingsService::IncidentSettings &settings);

private:
    void load();
    void save() const;
    void emitAllSignals();

    AppSettings m_settings;
    QSettings *m_qsettings{nullptr};
};

#endif // SERVICES_SETTINGSSERVICE_H
