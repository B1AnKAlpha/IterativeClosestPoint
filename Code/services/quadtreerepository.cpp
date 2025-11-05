#include "services/quadtreerepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QPointF>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace {
constexpr QLatin1String kPrimaryTimestampFormat("yyyy-MM-dd HH:mm:ss");
constexpr QLatin1String kFallbackTimestampFormat("yyyy/MM/dd HH:mm:ss");
constexpr double kInvalidCoordinate = 999.0;

struct ExtentAccumulator {
    double minLon{std::numeric_limits<double>::max()};
    double maxLon{std::numeric_limits<double>::lowest()};
    double minLat{std::numeric_limits<double>::max()};
    double maxLat{std::numeric_limits<double>::lowest()};

    void include(double lon, double lat)
    {
        minLon = std::min(minLon, lon);
        maxLon = std::max(maxLon, lon);
        minLat = std::min(minLat, lat);
        maxLat = std::max(maxLat, lat);
    }

    QRectF toRect() const
    {
        if (minLon == std::numeric_limits<double>::max() ||
            minLat == std::numeric_limits<double>::max()) {
            return {};
        }
        return QRectF(QPointF(minLon, minLat), QPointF(maxLon, maxLat));
    }
};

bool isCoordinateValid(double value)
{
    return !std::isnan(value) && value != kInvalidCoordinate && std::abs(value) <= 360.0;
}

} // namespace

QuadTreeRepository::QuadTreeRepository(QObject *parent)
    : QObject(parent)
{
}

bool QuadTreeRepository::loadFromDirectory(const QString &directoryPath, int depth)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        emit errorOccurred(tr("目录不存在: %1").arg(directoryPath));
        return false;
    }

    const QStringList filters{QStringLiteral("*.txt"), QStringLiteral("*.csv"), QStringLiteral("*.dat")};
    QStringList files;
    for (const auto &filter : filters) {
        const auto matches = dir.entryList(QStringList{filter}, QDir::Files | QDir::Readable);
        for (const auto &file : matches) {
            files << dir.absoluteFilePath(file);
        }
    }

    files.removeDuplicates();
    if (files.isEmpty()) {
        emit errorOccurred(tr("未在目录中找到可用数据文件"));
        return false;
    }

    return loadFromFiles(files, depth);
}

bool QuadTreeRepository::loadFromFiles(const QStringList &filePaths, int depth)
{
    if (filePaths.isEmpty()) {
        emit errorOccurred(tr("未指定任何数据文件"));
        return false;
    }

    m_storage.clear();
    m_root.reset();
    m_loadedFiles = filePaths;
    m_stats = LoadStatistics{};
    m_stats.fileCount = filePaths.size();
    m_stats.depth = depth;
    m_depth = depth;
    ExtentAccumulator extent;

    int processed = 0;
    for (const auto &path : filePaths) {
        if (!ingestFile(path)) {
            emit errorOccurred(tr("读取文件失败: %1").arg(path));
            continue;
        }
        ++processed;
        emit loadProgress(processed, filePaths.size());
        if ((processed % 10) == 0) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        }
    }

    if (m_storage.empty()) {
        emit errorOccurred(tr("数据文件为空或格式不正确"));
        m_root.reset();
        return false;
    }

    // 重新计算全局范围
    for (const auto &record : m_storage) {
        if (!record) {
            continue;
        }
        extent.include(record->longitude, record->latitude);
    }
    m_stats.extent = extent.toRect();
    m_stats.recordCount = static_cast<int>(m_storage.size());

    return buildIndex(depth);
}

bool QuadTreeRepository::isReady() const
{
    return m_root != nullptr && !m_storage.empty();
}

QStringList QuadTreeRepository::loadedFiles() const
{
    return m_loadedFiles;
}

QuadTreeRepository::TrajectoryTrack QuadTreeRepository::trajectoryForVehicle(int vehicleId,
                                                                             const QDateTime &startTime,
                                                                             const QDateTime &endTime,
                                                                             const QRectF &spatialConstraint) const
{
    TrajectoryTrack track;
    if (!isReady() || vehicleId <= 0) {
        return track;
    }

    QVector<const GPSdata*> filtered;
    QVector<GPSdata*> candidates;
    if (!spatialConstraint.isNull()) {
        candidates = collectRange(spatialConstraint);
        filtered.reserve(candidates.size());
    }

    if (spatialConstraint.isNull()) {
        // 当未指定空间范围时，遍历所有数据
        for (const auto &record : m_storage) {
            if (!record) {
                continue;
            }
            if (record->id != vehicleId) {
                continue;
            }
            if (!withinTimeRange(record.get(), startTime, endTime)) {
                continue;
            }
            filtered.append(record.get());
        }
    } else {
        for (auto gps : candidates) {
            if (!gps || gps->id != vehicleId) {
                continue;
            }
            if (!withinTimeRange(gps, startTime, endTime)) {
                continue;
            }
            filtered.append(gps);
        }
    }

    std::sort(filtered.begin(), filtered.end(), [](const GPSdata *lhs, const GPSdata *rhs) {
        return QuadTreeRepository::parseTimestamp(lhs->time) < QuadTreeRepository::parseTimestamp(rhs->time);
    });

    track.points.reserve(filtered.size());
    track.timeline.reserve(filtered.size());
    for (auto gps : filtered) {
        track.points.append(QPointF(gps->longitude, gps->latitude));
        track.timeline.append(parseTimestamp(gps->time));
    }

    return track;
}

QVector<TrajectoryPoint> QuadTreeRepository::pointsInRegion(const QRectF &bounds,
                                                            const QDateTime &startTime,
                                                            const QDateTime &endTime) const
{
    QVector<TrajectoryPoint> points;
    if (!isReady() || bounds.isNull()) {
        return points;
    }

    const auto matches = collectRange(bounds);
    points.reserve(matches.size());
    for (auto gps : matches) {
        if (!gps) {
            continue;
        }
        if (!withinTimeRange(gps, startTime, endTime)) {
            continue;
        }
        TrajectoryPoint point;
        point.vehicleId = gps->id;
        point.rawTimestamp = QString::fromStdString(gps->time);
        point.timestamp = parseTimestamp(gps->time);
        point.longitude = gps->longitude;
        point.latitude = gps->latitude;
        points.append(point);
    }

    std::sort(points.begin(), points.end(), [](const TrajectoryPoint &lhs, const TrajectoryPoint &rhs) {
        if (lhs.vehicleId == rhs.vehicleId) {
            return lhs.timestamp < rhs.timestamp;
        }
        return lhs.vehicleId < rhs.vehicleId;
    });

    return points;
}

QuadTreeRepository::LoadStatistics QuadTreeRepository::statistics() const
{
    return m_stats;
}

bool QuadTreeRepository::buildIndex(int depth)
{
    if (m_storage.empty()) {
        return false;
    }

    if (m_stats.extent.isNull()) {
        ExtentAccumulator accumulator;
        for (const auto &record : m_storage) {
            if (!record) {
                continue;
            }
            accumulator.include(record->longitude, record->latitude);
        }
        m_stats.extent = accumulator.toRect();
    }

    auto bounds = std::make_unique<Rectangle>();
    const QRectF normalized = m_stats.extent.normalized();
    bounds->bottom_left = std::make_pair(normalized.left(), normalized.top());
    bounds->top_right = std::make_pair(normalized.right(), normalized.bottom());

    m_root = std::make_unique<QuadNode>(bounds.release());
    for (const auto &record : m_storage) {
        if (!record) {
            continue;
        }
        m_root->InsertNode(record.get(), m_depth);
    }

    emit loadCompleted(m_stats);
    return true;
}

bool QuadTreeRepository::ingestFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    int lineNumber = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        ++lineNumber;
        if (line.isEmpty()) {
            continue;
        }
        const auto parsed = parseRecord(line);
        if (!parsed.has_value()) {
            emit errorOccurred(tr("解析失败 %1:%2").arg(filePath).arg(lineNumber));
            continue;
        }
        const auto &point = parsed.value();
        if (!isCoordinateValid(point.longitude) || !isCoordinateValid(point.latitude)) {
            continue;
        }
        auto gps = std::make_unique<GPSdata>(point.vehicleId,
                                             point.rawTimestamp.toStdString(),
                                             point.longitude,
                                             point.latitude);
        m_storage.emplace_back(std::move(gps));
    }
    return true;
}

std::optional<TrajectoryPoint> QuadTreeRepository::parseRecord(const QString &line)
{
    const auto parts = line.split(QLatin1Char(','));
    if (parts.size() < 4) {
        return std::nullopt;
    }

    bool okId = false;
    const int vehicleId = parts.at(0).toInt(&okId);
    if (!okId) {
        return std::nullopt;
    }

    const QString rawTs = parts.at(1).trimmed();
    const QDateTime ts = QDateTime::fromString(rawTs, kPrimaryTimestampFormat);
    QDateTime timestamp = ts.isValid() ? ts : QDateTime::fromString(rawTs, kFallbackTimestampFormat);

    bool okLon = false;
    const double lon = parts.at(2).toDouble(&okLon);
    bool okLat = false;
    const double lat = parts.at(3).toDouble(&okLat);

    if (!okLon || !okLat) {
        return std::nullopt;
    }

    TrajectoryPoint point;
    point.vehicleId = vehicleId;
    point.timestamp = timestamp;
    point.rawTimestamp = rawTs;
    point.longitude = lon;
    point.latitude = lat;
    return point;
}

Rectangle QuadTreeRepository::makeRectangle(const QRectF &rect)
{
    const QRectF normalized = rect.normalized();
    Rectangle bounding;
    bounding.top_right = std::make_pair(normalized.right(), normalized.bottom());
    bounding.bottom_left = std::make_pair(normalized.left(), normalized.top());
    return bounding;
}

QVector<GPSdata*> QuadTreeRepository::collectRange(const QRectF &bounds) const
{
    if (!m_root) {
        return {};
    }
    Rectangle rect = makeRectangle(bounds.normalized());
    const auto matches = m_root->AreaSearch(&rect);
    QVector<GPSdata*> vector;
    vector.reserve(matches.size());
    for (auto gps : matches) {
        vector.append(gps);
    }
    return vector;
}

bool QuadTreeRepository::withinTimeRange(const GPSdata *data,
                                         const QDateTime &startTime,
                                         const QDateTime &endTime)
{
    if (!data) {
        return false;
    }
    if (!startTime.isValid() && !endTime.isValid()) {
        return true;
    }
    const auto timestamp = parseTimestamp(data->time);
    if (!timestamp.isValid()) {
        return !startTime.isValid() && !endTime.isValid();
    }
    if (startTime.isValid() && timestamp < startTime) {
        return false;
    }
    if (endTime.isValid() && timestamp > endTime) {
        return false;
    }
    return true;
}

QDateTime QuadTreeRepository::parseTimestamp(const std::string &timestamp)
{
    const QString ts = QString::fromStdString(timestamp);
    QDateTime parsed = QDateTime::fromString(ts, kPrimaryTimestampFormat);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(ts, kFallbackTimestampFormat);
    }
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(ts, Qt::ISODate);
    }
    return parsed;
}
