#ifndef SERVICES_TRAJECTORYSERVICE_H
#define SERVICES_TRAJECTORYSERVICE_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QHash>
#include <QStringList>

#include "common/queryparameters.h"
#include "services/quadtreerepository.h"

class TrajectoryService : public QObject
{
    Q_OBJECT

public:
    explicit TrajectoryService(QObject *parent = nullptr);

    struct TrajectoryTrackData {
        QVector<QPointF> points;
        QVector<QDateTime> timeline;
    };

    bool isReady() const;
    bool loadFromDirectory(const QString &directoryPath, int depth = 8);
    bool loadFromFiles(const QStringList &filePaths, int depth = 8);
    QuadTreeRepository::LoadStatistics repositoryStatistics() const;
    QStringList loadedFiles() const;

    TrajectoryTrackData track(const QString &trajectoryId) const;
    QStringList availableTracks() const;
    QVector<TrajectoryPoint> pointsOnDate(const QDate &date) const;

signals:
    void queryCompleted(const QueryParameters &parameters, int resultCount);
    void trajectoryPreviewReady(const QString &trajectoryId, const QVector<QPointF> &points);
    void trajectoryTrackReady(const QString &trajectoryId, const QVector<QPointF> &points,
                              const QVector<QDateTime> &timeline);
    void dataLoadProgress(int processedFiles, int totalFiles);
    void dataLoadCompleted(const QuadTreeRepository::LoadStatistics &stats);
    void dataLoadFailed(const QString &message);
    void queryFailed(const QString &message);

public slots:
    void runQuery(const QueryParameters &parameters);

private:
    bool ensureRepositoryReady();
    TrajectoryTrackData buildTrackFromQuery(const QueryParameters &parameters, int &resultCount);
    QRectF resolveBounds(const QueryParameters &parameters) const;
    TrajectoryTrackData convertTrajectory(const QString &vehicleId,
                                          const QuadTreeRepository::TrajectoryTrack &trackData,
                                          int &resultCount);
    QVector<QPointF> toPointsOnly(const QVector<TrajectoryPoint> &points) const;

    QuadTreeRepository *m_repository{nullptr};
    QHash<QString, TrajectoryTrackData> m_tracks;
};

#endif // SERVICES_TRAJECTORYSERVICE_H
