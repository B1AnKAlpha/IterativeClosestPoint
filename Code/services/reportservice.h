#ifndef SERVICES_REPORTSERVICE_H
#define SERVICES_REPORTSERVICE_H

#include <QObject>
#include <QDate>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>

#include "querymanager.h"

class TrajectoryService;

class ReportService : public QObject
{
    Q_OBJECT

public:
    struct ReportConfig {
        QString schedule;
        QStringList owners;
        int archiveDays{180};
    };

    struct VehicleMetrics {
        QString vehicleId;
        int pointCount{0};
        double distanceKm{0.0};
        double averageSpeedKmH{0.0};
    };

    struct HourlyStat {
        QString label;
        int hits{0};
    };

    struct DailySummary {
        QDate date;
        int totalTrips{0};
        double totalDistanceKm{0.0};
        QVector<QString> activeVehicles;
        double averageSpeedKmH{0.0};
        int totalPoints{0};
        int uniqueVehicles{0};
        QVector<VehicleMetrics> vehicleDetails;
        QVector<HourlyStat> peakHourStats;
    };

    explicit ReportService(QObject *parent = nullptr);

    void setDataSources(QueryManager *queryManager, TrajectoryService *trajectoryService);

    ReportConfig config() const;
    void updateSchedule(const QString &schedule);
    void updateOwners(const QStringList &owners);
    void updateArchiveDays(int days);

    DailySummary buildDailySummary(const QDate &date) const;
    bool generateDailyOverview(const QDate &date, const QString &outputPath);

signals:
    void reportGenerated(const QString &outputPath);
    void reportGenerationFailed(const QString &reason);
    void configChanged(const ReportConfig &config);

private:
    QString configFilePath() const;
    void loadConfig();
    void saveConfig() const;
    void ensureDataSources() const;
    bool writePdf(const DailySummary &summary, const QString &outputPath) const;
    DailySummary computeTrajectorySummary(const QDate &date) const;
    static double computeHaversineKm(const QPointF &a, const QPointF &b);

    ReportConfig m_config;
    QueryManager *m_queryManager{nullptr};
    TrajectoryService *m_trajectoryService{nullptr};
};

#endif // SERVICES_REPORTSERVICE_H
