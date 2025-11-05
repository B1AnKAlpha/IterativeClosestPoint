#include "services/trajectoryservice.h"

#include <QDateTime>
#include <QRectF>
#include <QtGlobal>

TrajectoryService::TrajectoryService(QObject *parent)
    : QObject(parent)
{
    m_repository = new QuadTreeRepository(this);
    connect(m_repository, &QuadTreeRepository::loadProgress, this, &TrajectoryService::dataLoadProgress);
    connect(m_repository, &QuadTreeRepository::loadCompleted, this, &TrajectoryService::dataLoadCompleted);
    connect(m_repository, &QuadTreeRepository::errorOccurred, this, &TrajectoryService::dataLoadFailed);
}

bool TrajectoryService::isReady() const
{
    return m_repository && m_repository->isReady();
}

bool TrajectoryService::loadFromDirectory(const QString &directoryPath, int depth)
{
    if (!m_repository) {
        m_repository = new QuadTreeRepository(this);
    }
    return m_repository->loadFromDirectory(directoryPath, depth);
}

bool TrajectoryService::loadFromFiles(const QStringList &filePaths, int depth)
{
    if (!m_repository) {
        m_repository = new QuadTreeRepository(this);
    }
    return m_repository->loadFromFiles(filePaths, depth);
}

QuadTreeRepository::LoadStatistics TrajectoryService::repositoryStatistics() const
{
    return m_repository ? m_repository->statistics() : QuadTreeRepository::LoadStatistics{};
}

QStringList TrajectoryService::loadedFiles() const
{
    return m_repository ? m_repository->loadedFiles() : QStringList{};
}

void TrajectoryService::runQuery(const QueryParameters &parameters)
{
    if (!ensureRepositoryReady()) {
        emit queryFailed(tr("轨迹库尚未初始化"));
        emit queryCompleted(parameters, 0);
        return;
    }

    int resultCount = 0;
    const auto track = buildTrackFromQuery(parameters, resultCount);
    emit queryCompleted(parameters, resultCount);

    if (resultCount == 0) {
        emit queryFailed(tr("未检索到符合条件的轨迹"));
        return;
    }

    QString id = parameters.vehicleId;
    if (id.isEmpty()) {
        id = QStringLiteral("region-%1").arg(QDateTime::currentMSecsSinceEpoch());
    }
    m_tracks.insert(id, track);
    emit trajectoryPreviewReady(id, track.points);
    emit trajectoryTrackReady(id, track.points, track.timeline);
}

TrajectoryService::TrajectoryTrackData TrajectoryService::buildTrackFromQuery(const QueryParameters &parameters, int &resultCount)
{
    TrajectoryTrackData track;
    resultCount = 0;
    const QRectF bounds = resolveBounds(parameters);
    const QDateTime start = parameters.startTime;
    const QDateTime end = parameters.endTime;
    const QueryType type = parameters.type;

    if (type == QueryType::Area || parameters.vehicleId.isEmpty()) {
        if (bounds.isNull()) {
            emit queryFailed(tr("请提供完整的经纬度范围"));
            return track;
        }
        const auto regionPoints = m_repository->pointsInRegion(bounds, start, end);
        resultCount = regionPoints.size();
        track.points = toPointsOnly(regionPoints);
        track.timeline.reserve(regionPoints.size());
        for (const auto &pt : regionPoints) {
            track.timeline.append(pt.timestamp);
        }
        return track;
    }

    bool ok = false;
    const int vehicleId = parameters.vehicleId.toInt(&ok);
    if (!ok) {
        emit queryFailed(tr("车辆编号需为数字"));
        return track;
    }

    const auto data = m_repository->trajectoryForVehicle(vehicleId, start, end, bounds);
    track = convertTrajectory(parameters.vehicleId, data, resultCount);
    return track;
}

QRectF TrajectoryService::resolveBounds(const QueryParameters &parameters) const
{
    bool okMinLon = false;
    bool okMaxLon = false;
    bool okMinLat = false;
    bool okMaxLat = false;

    const double lonMin = parameters.longitudeMin.toDouble(&okMinLon);
    const double lonMax = parameters.longitudeMax.toDouble(&okMaxLon);
    const double latMin = parameters.latitudeMin.toDouble(&okMinLat);
    const double latMax = parameters.latitudeMax.toDouble(&okMaxLat);

    if (!okMinLon || !okMaxLon || !okMinLat || !okMaxLat) {
        return QRectF();
    }

    const QRectF bounds(QPointF(lonMin, latMin), QPointF(lonMax, latMax));
    const QRectF normalized = bounds.normalized();
    if (normalized.isEmpty()) {
        return QRectF();
    }
    return normalized;
}

TrajectoryService::TrajectoryTrackData TrajectoryService::track(const QString &trajectoryId) const
{
    return m_tracks.value(trajectoryId);
}

QStringList TrajectoryService::availableTracks() const
{
    return m_tracks.keys();
}

QVector<TrajectoryPoint> TrajectoryService::pointsOnDate(const QDate &date) const
{
    if (!m_repository || !m_repository->isReady() || !date.isValid()) {
        return {};
    }
    const auto stats = m_repository->statistics();
    if (stats.extent.isNull()) {
        return {};
    }
    const QDateTime start(date, QTime(0, 0, 0));
    const QDateTime end(date, QTime(23, 59, 59));
    return m_repository->pointsInRegion(stats.extent, start, end);
}

bool TrajectoryService::ensureRepositoryReady()
{
    if (isReady()) {
        return true;
    }
    if (m_repository && !m_repository->loadedFiles().isEmpty()) {
        return m_repository->loadFromFiles(m_repository->loadedFiles());
    }
    return false;
}

TrajectoryService::TrajectoryTrackData TrajectoryService::convertTrajectory(const QString &vehicleId,
                                                                            const QuadTreeRepository::TrajectoryTrack &trackData,
                                                                            int &resultCount)
{
    TrajectoryTrackData track;
    track.points = trackData.points;
    track.timeline = trackData.timeline;
    resultCount = track.points.size();
    if (resultCount == 0) {
        m_tracks.remove(vehicleId);
    }
    return track;
}

QVector<QPointF> TrajectoryService::toPointsOnly(const QVector<TrajectoryPoint> &points) const
{
    QVector<QPointF> pts;
    pts.reserve(points.size());
    for (const auto &p : points) {
        pts.append(QPointF(p.longitude, p.latitude));
    }
    return pts;
}
