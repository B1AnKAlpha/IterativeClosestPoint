#ifndef SERVICES_STATISTICSERVICE_H
#define SERVICES_STATISTICSERVICE_H

#include <QObject>
#include <QVariantMap>

#include "querymanager.h"

class StatisticService : public QObject
{
    Q_OBJECT

public:
    explicit StatisticService(QObject *parent = nullptr);

    QVariantMap currentMetrics(const QVector<QueryRecord> &records) const;
};

#endif // SERVICES_STATISTICSERVICE_H
