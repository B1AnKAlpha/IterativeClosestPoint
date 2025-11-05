#ifndef UI_PAGES_INCIDENTPAGE_H
#define UI_PAGES_INCIDENTPAGE_H

#include <QWidget>
#include <QStringList>
#include <QList>
#include <QDateTime>

#include "services/incidentservice.h"

class QListWidget;
class InfoCard;

class IncidentPage : public QWidget
{
    Q_OBJECT

public:
    explicit IncidentPage(QWidget *parent = nullptr);

    void setIncidentService(IncidentService *service);
    void refreshIncidents(const QStringList &incidents);
    void initializeOverview();

public slots:
    void onIncidentFeed(const QList<IncidentService::IncidentRecord> &records);
    void onIncidentAdded(const IncidentService::IncidentRecord &record);
    void onIncidentUpdated(const IncidentService::IncidentRecord &record);

private:
    void updateCards(const QList<IncidentService::IncidentRecord> &records);
    void rebuildList();

    InfoCard *m_todayCard{nullptr};
    InfoCard *m_resolvedCard{nullptr};
    InfoCard *m_pendingCard{nullptr};
    QListWidget *m_incidentList{nullptr};
    IncidentService *m_service{nullptr};
    QList<IncidentService::IncidentRecord> m_records;
};

#endif // UI_PAGES_INCIDENTPAGE_H
