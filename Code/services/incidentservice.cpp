#include "services/incidentservice.h"

#include <QAbstractSocket>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketProtocol>

namespace {
constexpr int kMockIntervalMs = 20000;
constexpr int kRetryIntervalMs = 8000;
constexpr int kMaxIncidents = 100;
}

IncidentService::IncidentService(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_mockTimer(new QTimer(this))
    , m_retryTimer(new QTimer(this))
{
    m_mockTimer->setInterval(kMockIntervalMs);
    connect(m_mockTimer, &QTimer::timeout, this, &IncidentService::refreshFeed);

    m_retryTimer->setInterval(kRetryIntervalMs);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &IncidentService::connectEndpoint);

    connect(m_socket, &QWebSocket::connected, this, [this]() {
        ensureMockTimer(false);
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, &IncidentService::handleMessage);
    connect(m_socket, &QWebSocket::disconnected, this, [this]() {
        if (!m_useMock) {
            scheduleReconnect();
            ensureMockTimer(true);
        }
    });
    connect(m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (!m_useMock) {
            scheduleReconnect();
            ensureMockTimer(true);
        }
    });

    ensureMockTimer(true);
}

QList<IncidentService::IncidentRecord> IncidentService::incidents() const
{
    return m_activeIncidents;
}

void IncidentService::applySettings(const IncidentConnectionSettings &settings)
{
    m_useMock = settings.useMock;
    m_endpoint = QUrl(settings.endpoint);

    if (m_useMock || !m_endpoint.isValid()) {
        disconnectEndpoint();
        ensureMockTimer(true);
    } else {
        ensureMockTimer(false);
        connectEndpoint();
    }
}

void IncidentService::acknowledgeIncident(const QString &incidentId)
{
    for (auto &record : m_activeIncidents) {
        if (record.id == incidentId) {
            record.status = tr("已处理");
            emit incidentUpdated(record);
            break;
        }
    }

    if (!m_useMock && m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject obj;
        obj.insert(QStringLiteral("type"), QStringLiteral("ack"));
        obj.insert(QStringLiteral("id"), incidentId);
        const auto payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        m_socket->sendTextMessage(QString::fromUtf8(payload));
    }
}

void IncidentService::refreshFeed()
{
    if (!m_useMock) {
        return;
    }

    const auto incident = buildMockIncident();
    m_activeIncidents.prepend(incident);
    while (m_activeIncidents.size() > 12) {
        m_activeIncidents.removeLast();
    }
    emit incidentAdded(incident);
    emit incidentFeedReady(m_activeIncidents);
}

void IncidentService::connectEndpoint()
{
    if (m_useMock || !m_endpoint.isValid()) {
        ensureMockTimer(true);
        return;
    }

    if (!m_socket) {
        return;
    }

    if (m_socket->state() == QAbstractSocket::ConnectedState && m_socket->requestUrl() == m_endpoint) {
        return;
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }

    m_socket->open(m_endpoint);
}

void IncidentService::disconnectEndpoint()
{
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }
}

void IncidentService::handleMessage(const QString &message)
{
    const auto doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();
    const QJsonValue payload = obj.value(QStringLiteral("payload"));

    if (type == QLatin1String("bootstrap") && payload.isArray()) {
        QList<IncidentRecord> records;
        const auto array = payload.toArray();
        records.reserve(array.size());
        for (const auto &item : array) {
            if (item.isObject()) {
                records.append(fromJson(item.toObject()));
            }
        }
        processBootstrap(records);
        ensureMockTimer(false);
    } else if (type == QLatin1String("incident") && payload.isObject()) {
        const auto record = fromJson(payload.toObject());
        m_activeIncidents.prepend(record);
        while (m_activeIncidents.size() > kMaxIncidents) {
            m_activeIncidents.removeLast();
        }
        emit incidentAdded(record);
        emit incidentFeedReady(m_activeIncidents);
    } else if (type == QLatin1String("update") && payload.isObject()) {
        const auto updated = fromJson(payload.toObject());
        for (auto &record : m_activeIncidents) {
            if (record.id == updated.id) {
                record = updated;
                emit incidentUpdated(record);
                break;
            }
        }
        emit incidentFeedReady(m_activeIncidents);
    }
}

void IncidentService::processBootstrap(const QList<IncidentRecord> &records)
{
    m_activeIncidents = records;
    emit incidentFeedReady(m_activeIncidents);
}

IncidentService::IncidentRecord IncidentService::fromJson(const QJsonObject &obj)
{
    IncidentRecord record;
    record.id = obj.value(QStringLiteral("id")).toString();
    record.level = obj.value(QStringLiteral("level")).toString();
    record.vehicleId = obj.value(QStringLiteral("vehicleId")).toString();
    record.message = obj.value(QStringLiteral("message")).toString();
    record.status = obj.value(QStringLiteral("status")).toString();
    const QString timestamp = obj.value(QStringLiteral("timestamp")).toString();
    record.timestamp = QDateTime::fromString(timestamp, Qt::ISODate);
    if (!record.timestamp.isValid()) {
        record.timestamp = QDateTime::currentDateTime();
    }
    if (record.status.isEmpty()) {
        record.status = QObject::tr("待确认");
    }
    return record;
}

IncidentService::IncidentRecord IncidentService::buildMockIncident()
{
    static const QStringList levels{QObject::tr("高"), QObject::tr("中"), QObject::tr("低")};
    static const QStringList vehicles{QStringLiteral("A231"), QStringLiteral("H552"),
                                      QStringLiteral("L878"), QStringLiteral("M104")};
    static const QStringList messages{
        QObject::tr("持续超速超过 15km/h"),
        QObject::tr("偏离指定区域 350m"),
        QObject::tr("通信延迟异常"),
        QObject::tr("燃油不足 10%"),
    };

    IncidentRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.level = levels.at(QRandomGenerator::global()->bounded(levels.size()));
    record.vehicleId = vehicles.at(QRandomGenerator::global()->bounded(vehicles.size()));
    record.message = messages.at(QRandomGenerator::global()->bounded(messages.size()));
    record.timestamp = QDateTime::currentDateTime();
    record.status = QObject::tr("待确认");
    return record;
}

void IncidentService::ensureMockTimer(bool enabled)
{
    if (!m_mockTimer) {
        return;
    }
    if (enabled) {
        if (!m_mockTimer->isActive()) {
            refreshFeed();
            m_mockTimer->start();
        }
    } else {
        m_mockTimer->stop();
    }
}

void IncidentService::scheduleReconnect()
{
    if (!m_retryTimer || m_useMock) {
        return;
    }
    if (!m_retryTimer->isActive()) {
        m_retryTimer->start();
    }
}
