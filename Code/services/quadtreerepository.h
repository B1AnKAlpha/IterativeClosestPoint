#ifndef SERVICES_QUADTREEREPOSITORY_H
#define SERVICES_QUADTREEREPOSITORY_H

#include <QObject>
#include <QDateTime>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>
#include <optional>
#include <vector>

#include "quadtree.h"

struct TrajectoryPoint
{
    int vehicleId{0};
    QDateTime timestamp;
    QString rawTimestamp;
    double longitude{0.0};
    double latitude{0.0};
};

class QuadTreeRepository : public QObject
{
    Q_OBJECT

public:
    explicit QuadTreeRepository(QObject *parent = nullptr);

    struct LoadStatistics {
        int fileCount{0};
        int recordCount{0};
        QRectF extent;
        int depth{0};
    };

    struct TrajectoryTrack {
        QVector<QPointF> points;
        QVector<QDateTime> timeline;
    };

    bool loadFromDirectory(const QString &directoryPath, int depth = 8);
    bool loadFromFiles(const QStringList &filePaths, int depth = 8);
    bool isReady() const;
    QStringList loadedFiles() const;

    TrajectoryTrack trajectoryForVehicle(int vehicleId,
                                         const QDateTime &startTime,
                                         const QDateTime &endTime,
                                         const QRectF &spatialConstraint = QRectF()) const;

    QVector<TrajectoryPoint> pointsInRegion(const QRectF &bounds,
                                            const QDateTime &startTime = QDateTime(),
                                            const QDateTime &endTime = QDateTime()) const;

    LoadStatistics statistics() const;

signals:
    void loadProgress(int processedFiles, int totalFiles);
    void loadCompleted(const QuadTreeRepository::LoadStatistics &stats);
    void errorOccurred(const QString &message);

private:
    bool buildIndex(int depth);
    bool ingestFile(const QString &filePath);
    static std::optional<TrajectoryPoint> parseRecord(const QString &line);
    static Rectangle makeRectangle(const QRectF &rect);
    QVector<GPSdata*> collectRange(const QRectF &bounds) const;
    static bool withinTimeRange(const GPSdata *data,
                                const QDateTime &startTime,
                                const QDateTime &endTime);
    static QDateTime parseTimestamp(const std::string &timestamp);

    std::unique_ptr<QuadNode> m_root;
    std::vector<std::unique_ptr<GPSdata>> m_storage;
    LoadStatistics m_stats;
    QStringList m_loadedFiles;
    int m_depth{8};
};

#endif // SERVICES_QUADTREEREPOSITORY_H
