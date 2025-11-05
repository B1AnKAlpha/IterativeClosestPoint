#include "services/statisticservice.h"

#include <QLocale>
#include <algorithm>

StatisticService::StatisticService(QObject *parent)
    : QObject(parent)
{
}

QVariantMap StatisticService::currentMetrics(const QVector<QueryRecord> &records) const
{
    const int activeTrajectories = records.size();
    const int alerts = std::count_if(records.cbegin(), records.cend(), [](const QueryRecord &record) {
        return record.resultCount == 0;
    });

    QVariantMap metrics;
    metrics.insert(QStringLiteral("activeTrajectories"), activeTrajectories);
    metrics.insert(QStringLiteral("alerts"), alerts);
    metrics.insert(QStringLiteral("dataVolume"), QLocale().toString(activeTrajectories * 1500) + QStringLiteral(" pts"));
    metrics.insert(QStringLiteral("throughput"), QStringLiteral("%1/s").arg(activeTrajectories * 3));
    metrics.insert(QStringLiteral("latency"), QStringLiteral("%1 ms").arg(300 - qMin(activeTrajectories, 200)));
    return metrics;
}
