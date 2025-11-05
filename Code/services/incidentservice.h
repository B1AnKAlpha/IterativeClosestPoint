#ifndef SERVICES_INCIDENTSERVICE_H
#define SERVICES_INCIDENTSERVICE_H

#include <QObject>
#include <QDateTime>
#include <QStringList>
#include <QTimer>
#include <QUrl>

class QWebSocket;
class QJsonObject;

class IncidentService : public QObject
{
    Q_OBJECT

public:
    explicit IncidentService(QObject *parent = nullptr);

    struct IncidentConnectionSettings {
        QString endpoint;
        bool useMock{true};
    };

    struct IncidentRecord {
        QString id;
        QString level;
        QString vehicleId;
        QString message;
        QDateTime timestamp;
        QString status;
    };

    QList<IncidentRecord> incidents() const;

public slots:
    void applySettings(const IncidentConnectionSettings &settings);

signals:
    void incidentAdded(const IncidentRecord &record);
    void incidentUpdated(const IncidentRecord &record);
    void incidentFeedReady(const QList<IncidentRecord> &records);

public slots:
    void acknowledgeIncident(const QString &incidentId);
    void refreshFeed();

private:
    void connectEndpoint();
    void disconnectEndpoint();
    void handleMessage(const QString &message);
    void processBootstrap(const QList<IncidentRecord> &records);
    static IncidentRecord fromJson(const QJsonObject &obj);
    IncidentRecord buildMockIncident();
    void ensureMockTimer(bool enabled);
    void scheduleReconnect();

    QList<IncidentRecord> m_activeIncidents;
    QWebSocket *m_socket{nullptr};
    QTimer *m_mockTimer{nullptr};
    QTimer *m_retryTimer{nullptr};
    bool m_useMock{true};
    QUrl m_endpoint;
};

#endif // SERVICES_INCIDENTSERVICE_H