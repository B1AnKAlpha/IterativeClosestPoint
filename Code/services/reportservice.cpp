#include "services/reportservice.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfWriter>
#include <QPointF>
#include <QStandardPaths>
#include <QStringList>
#include <QTextDocument>
#include <QtGlobal>
#include <QtMath>

#include <algorithm>

#include "services/trajectoryservice.h"

namespace {
constexpr qreal kDocumentMargin = 24.0;
constexpr int kMaxActiveDisplay = 6;
constexpr int kDefaultArchiveDays = 180;

QString ensureDirectory(const QString &path)
{
    QFileInfo info(path);
    if (!info.dir().exists()) {
        info.dir().mkpath(QStringLiteral("."));
    }
    return info.filePath();
}
}

ReportService::ReportService(QObject *parent)
    : QObject(parent)
{
    loadConfig();
}

void ReportService::setDataSources(QueryManager *queryManager, TrajectoryService *trajectoryService)
{
    m_queryManager = queryManager;
    m_trajectoryService = trajectoryService;
}

ReportService::ReportConfig ReportService::config() const
{
    return m_config;
}

void ReportService::updateSchedule(const QString &schedule)
{
    if (m_config.schedule == schedule) {
        return;
    }
    m_config.schedule = schedule;
    saveConfig();
    emit configChanged(m_config);
}

void ReportService::updateOwners(const QStringList &owners)
{
    if (m_config.owners == owners) {
        return;
    }
    m_config.owners = owners;
    saveConfig();
    emit configChanged(m_config);
}

void ReportService::updateArchiveDays(int days)
{
    if (days <= 0 || m_config.archiveDays == days) {
        return;
    }
    m_config.archiveDays = days;
    saveConfig();
    emit configChanged(m_config);
}

ReportService::DailySummary ReportService::buildDailySummary(const QDate &date) const
{
    DailySummary summary = computeTrajectorySummary(date);
    summary.date = date;
    return summary;
}

bool ReportService::generateDailyOverview(const QDate &date, const QString &outputPath)
{
    if (!m_trajectoryService || !m_trajectoryService->isReady()) {
        emit reportGenerationFailed(tr("轨迹数据尚未准备好"));
        return false;
    }

    DailySummary summary = buildDailySummary(date);
    if (summary.totalPoints == 0) {
        emit reportGenerationFailed(tr("指定日期没有可用轨迹数据"));
        return false;
    }

    const QString ensuredPath = ensureDirectory(outputPath);
    if (!writePdf(summary, ensuredPath)) {
        emit reportGenerationFailed(tr("PDF 写入失败"));
        return false;
    }

    emit reportGenerated(ensuredPath);
    return true;
}

QString ReportService::configFilePath() const
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        return QStringLiteral("report-config.json");
    }
    QDir dir(baseDir);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("report-config.json"));
}

void ReportService::loadConfig()
{
    m_config.archiveDays = kDefaultArchiveDays;

    QFile file(configFilePath());
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    m_config.schedule = obj.value(QStringLiteral("schedule")).toString();

    const QJsonValue ownersValue = obj.value(QStringLiteral("owners"));
    if (ownersValue.isArray()) {
        QStringList owners;
        const QJsonArray arr = ownersValue.toArray();
        owners.reserve(arr.size());
        for (const QJsonValue &value : arr) {
            owners.append(value.toString());
        }
        m_config.owners = owners;
    }

    const int archiveDays = obj.value(QStringLiteral("archiveDays")).toInt(kDefaultArchiveDays);
    if (archiveDays > 0) {
        m_config.archiveDays = archiveDays;
    }
}

void ReportService::saveConfig() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("schedule"), m_config.schedule);

    QJsonArray ownersArr;
    for (const QString &owner : m_config.owners) {
        ownersArr.append(owner);
    }
    obj.insert(QStringLiteral("owners"), ownersArr);
    obj.insert(QStringLiteral("archiveDays"), m_config.archiveDays);

    const QJsonDocument doc(obj);

    QFile file(configFilePath());
    const QString ensured = ensureDirectory(file.fileName());
    file.setFileName(ensured);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void ReportService::ensureDataSources() const
{
    Q_ASSERT_X(m_trajectoryService != nullptr, "ReportService", "TrajectoryService 未注入");
}

bool ReportService::writePdf(const DailySummary &summary, const QString &outputPath) const
{
    QPdfWriter writer(outputPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(kDocumentMargin, kDocumentMargin, kDocumentMargin, kDocumentMargin));
    writer.setResolution(120);

    QTextDocument document;
    document.setDocumentMargin(kDocumentMargin);
    QFont baseFont(QStringLiteral("Microsoft YaHei"));
    baseFont.setPointSize(11);
    document.setDefaultFont(baseFont);

    const QString scheduleText = m_config.schedule.trimmed().isEmpty() ? tr("未配置") : m_config.schedule;
    const QString ownersText = m_config.owners.isEmpty() ? tr("未指定") : m_config.owners.join(tr("、"));

    QStringList activeSlices;
    const int activeCount = summary.activeVehicles.size();
    const int displayCount = std::min(activeCount, kMaxActiveDisplay);
    for (int i = 0; i < displayCount; ++i) {
        activeSlices.append(summary.activeVehicles.at(i));
    }

    QString activeVehiclesText = activeSlices.isEmpty() ? tr("当日无活跃车辆数据")
                                                        : activeSlices.join(tr("、"));
    if (activeCount > displayCount) {
        activeVehiclesText += tr(" 等");
    }

    const QString activeSectionTitle = displayCount > 0
            ? tr("每日活跃车辆（展示前%1辆）").arg(displayCount)
            : tr("每日活跃车辆");

    QStringList vehicleRows;
    vehicleRows.reserve(summary.vehicleDetails.size());
    for (const VehicleMetrics &metrics : summary.vehicleDetails) {
        const QString distanceText = metrics.distanceKm > 0.0
                ? QString::number(metrics.distanceKm, 'f', 2)
                : tr("--");
        const QString speedText = metrics.averageSpeedKmH > 0.0
                ? QString::number(metrics.averageSpeedKmH, 'f', 1)
                : tr("--");

        vehicleRows.append(QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                                   .arg(metrics.vehicleId)
                                   .arg(metrics.pointCount)
                                   .arg(distanceText)
                                   .arg(speedText));
    }
    const QString vehicleTableRows = vehicleRows.isEmpty()
            ? QStringLiteral("<tr><td colspan=\"4\">%1</td></tr>").arg(tr("暂无车辆数据"))
            : vehicleRows.join(QString());

    QStringList peakRows;
    peakRows.reserve(summary.peakHourStats.size());
    for (const HourlyStat &stat : summary.peakHourStats) {
        peakRows.append(QStringLiteral("<tr><td>%1</td><td>%2</td></tr>")
                                .arg(stat.label)
                                .arg(stat.hits));
    }
    const QString peakHourRows = peakRows.isEmpty()
            ? QStringLiteral("<tr><td colspan=\"2\">%1</td></tr>").arg(tr("暂无高峰数据"))
            : peakRows.join(QString());

    const QString html = QStringLiteral(R"(
<html>
<head>
    <meta charset="utf-8" />
    <style>
        body { font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif; color: #1f2430; margin: 0; }
        .container { padding: 18pt 14pt 14pt; }
        .header { display: flex; flex-direction: column; align-items: center; gap: 8pt; margin-bottom: 16pt; }
        .title { font-size: 24pt; font-weight: 700; color: #152033; margin: 0; text-align: center; }
        .date-badge { background: #2a6cea; color: #fff; padding: 4pt 12pt; border-radius: 12pt; font-size: 11pt; letter-spacing: 0.4pt; }
        .meta { display: flex; flex-wrap: wrap; justify-content: center; gap: 14pt; font-size: 10pt; color: #69707a; margin-bottom: 24pt; text-align: center; }
        .meta span { display: inline-flex; align-items: center; justify-content: center; }
        .meta strong { color: #2a2f3a; margin-right: 4pt; }
        .stat-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140pt, 1fr)); gap: 14pt; }
        .card { background: #f6f8fb; border-radius: 12pt; padding: 12pt 14pt; box-shadow: 0 2pt 6pt rgba(21, 32, 51, 0.08); text-align: center; }
        .card .label { margin: 0; font-size: 9pt; color: #6d7885; text-transform: uppercase; letter-spacing: 0.8pt; }
        .card .value { margin: 6pt 0 0; font-size: 16pt; font-weight: 600; color: #202733; }
        .card .value.accent { color: #2a6cea; }
        section { margin-top: 30pt; }
        section + section { margin-top: 34pt; }
        h2 { font-size: 14pt; color: #1f2430; margin: 0 0 14pt; text-align: center; }
        table.striped { width: 85%; margin: 0 auto; border-collapse: collapse; font-size: 10pt; }
        table.striped thead { background: #f0f2f6; }
        table.striped th, table.striped td { padding: 8pt 10pt; border: 1px solid #d9dde6; text-align: center; }
        table.striped tbody tr:nth-child(even) { background: #f9fbff; }
        table.striped.small { width: 70%; }
        table.striped.small td, table.striped.small th { font-size: 9pt; }
        .note { font-size: 10pt; color: #6d7885; margin: 0; text-align: center; }
        .footer-note { margin-top: 30pt; font-size: 9pt; color: #9aa0ab; text-align: center; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1 class="title">网约车每日运行概览</h1>
            <div class="date-badge">%1</div>
        </div>
        <div class="meta">
            <span><strong>推送计划</strong>%2</span>
            <span><strong>责任人</strong>%3</span>
            <span><strong>自动归档</strong>%4 天</span>
            <span><strong>生成时间</strong>%5</span>
        </div>
        <div class="stat-grid">
            <div class="card"><p class="label">累计行驶距离</p><p class="value accent">%6 km</p></div>
            <div class="card"><p class="label">有效轨迹数</p><p class="value">%7</p></div>
            <div class="card"><p class="label">平均速度</p><p class="value">%8 km/h</p></div>
            <div class="card"><p class="label">活跃车辆数</p><p class="value">%9</p></div>
            <div class="card"><p class="label">轨迹点总数</p><p class="value">%10</p></div>
        </div>

        <section>
            <h2>%11</h2>
            <p class="note">%12</p>
        </section>

        <section>
            <h2>车辆运行明细</h2>
            <table class="striped">
                <thead><tr><th>车辆</th><th>轨迹点数</th><th>行驶距离 (km)</th><th>平均速度 (km/h)</th></tr></thead>
                <tbody>%13</tbody>
            </table>
        </section>

        <section>
            <h2>关键高峰时段</h2>
            <table class="striped small">
                <thead><tr><th>时段</th><th>轨迹点贡献</th></tr></thead>
                <tbody>%14</tbody>
            </table>
        </section>

        <p class="footer-note">%15</p>
    </div>
</body>
</html>
)")
            .arg(summary.date.toString(QStringLiteral("yyyy-MM-dd")))
            .arg(scheduleText)
            .arg(ownersText)
            .arg(QString::number(m_config.archiveDays))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")))
            .arg(QString::number(summary.totalDistanceKm, 'f', 2))
            .arg(QString::number(summary.totalTrips))
            .arg(QString::number(summary.averageSpeedKmH, 'f', 1))
            .arg(QString::number(summary.uniqueVehicles))
            .arg(QString::number(summary.totalPoints))
            .arg(activeSectionTitle)
            .arg(activeVehiclesText)
            .arg(vehicleTableRows)
            .arg(peakHourRows)
            .arg(tr("数据基于四叉树索引的轨迹记录自动统计生成。"));

    document.setHtml(html);
    document.print(&writer);
    return true;
}

ReportService::DailySummary ReportService::computeTrajectorySummary(const QDate &date) const
{
    DailySummary summary;
    summary.date = date;
    if (!m_trajectoryService || !m_trajectoryService->isReady()) {
        return summary;
    }

    const auto regionPoints = m_trajectoryService->pointsOnDate(date);
    if (regionPoints.isEmpty()) {
        return summary;
    }

    summary.totalPoints = regionPoints.size();

    QHash<int, QVector<TrajectoryPoint>> pointsPerVehicle;
    for (const auto &pt : regionPoints) {
        pointsPerVehicle[pt.vehicleId].append(pt);
    }

    summary.uniqueVehicles = pointsPerVehicle.size();
    summary.totalTrips = pointsPerVehicle.size();

    QHash<int, int> hourBuckets;
    double speedAccumulator = 0.0;
    int speedSamples = 0;

    for (auto it = pointsPerVehicle.cbegin(); it != pointsPerVehicle.cend(); ++it) {
        auto points = it.value();
        std::sort(points.begin(), points.end(), [](const TrajectoryPoint &lhs, const TrajectoryPoint &rhs) {
            return lhs.timestamp < rhs.timestamp;
        });

        if (points.size() < 2) {
            summary.activeVehicles.append(QString::number(it.key()));
            VehicleMetrics metrics;
            metrics.vehicleId = QString::number(it.key());
            metrics.pointCount = points.size();
            summary.vehicleDetails.append(metrics);
            continue;
        }

        double distance = 0.0;
        double totalHours = 0.0;
        for (int i = 1; i < points.size(); ++i) {
            distance += computeHaversineKm(QPointF(points.at(i - 1).longitude, points.at(i - 1).latitude),
                                           QPointF(points.at(i).longitude, points.at(i).latitude));
            const auto &prevTime = points.at(i - 1).timestamp;
            const auto &currTime = points.at(i).timestamp;
            if (prevTime.isValid() && currTime.isValid()) {
                totalHours += prevTime.msecsTo(currTime) / 3600000.0;
                const int hour = currTime.time().hour();
                hourBuckets[hour] += 1;
            }
        }

        if (distance > 0.0) {
            summary.totalDistanceKm += distance;
        }
        summary.activeVehicles.append(QString::number(it.key()));
        VehicleMetrics metrics;
        metrics.vehicleId = QString::number(it.key());
        metrics.pointCount = points.size();
        metrics.distanceKm = distance;
        if (totalHours > 0.0) {
            const double avgSpeed = distance / totalHours;
            speedAccumulator += avgSpeed;
            ++speedSamples;
            metrics.averageSpeedKmH = avgSpeed;
        }
        summary.vehicleDetails.append(metrics);
    }

    if (speedSamples > 0) {
        summary.averageSpeedKmH = speedAccumulator / static_cast<double>(speedSamples);
    }

    if (!summary.activeVehicles.isEmpty()) {
        std::sort(summary.activeVehicles.begin(), summary.activeVehicles.end());
        const auto uniqueEnd = std::unique(summary.activeVehicles.begin(), summary.activeVehicles.end());
        summary.activeVehicles.erase(uniqueEnd, summary.activeVehicles.end());
    }

    std::sort(summary.vehicleDetails.begin(), summary.vehicleDetails.end(), [](const VehicleMetrics &lhs, const VehicleMetrics &rhs) {
        if (!qFuzzyCompare(lhs.distanceKm + 1.0, rhs.distanceKm + 1.0)) {
            return lhs.distanceKm > rhs.distanceKm;
        }
        return lhs.pointCount > rhs.pointCount;
    });

    QList<int> sortedHours = hourBuckets.keys();
    std::sort(sortedHours.begin(), sortedHours.end(), [&hourBuckets](int lhs, int rhs) {
        return hourBuckets.value(lhs) > hourBuckets.value(rhs);
    });

    for (int hour : sortedHours) {
        HourlyStat stat;
        stat.label = tr("%1:00-%2:00").arg(hour, 2, 10, QLatin1Char('0')).arg((hour + 1) % 24, 2, 10, QLatin1Char('0'));
        stat.hits = hourBuckets.value(hour);
        summary.peakHourStats.append(stat);
    }

    return summary;
}

double ReportService::computeHaversineKm(const QPointF &a, const QPointF &b)
{
    static constexpr double kEarthRadiusKm = 6371.0;
    const double lat1 = qDegreesToRadians(a.y());
    const double lat2 = qDegreesToRadians(b.y());
    const double dLat = lat2 - lat1;
    const double dLon = qDegreesToRadians(b.x() - a.x());

    const double sinLat = qSin(dLat / 2.0);
    const double sinLon = qSin(dLon / 2.0);
    const double aa = sinLat * sinLat + qCos(lat1) * qCos(lat2) * sinLon * sinLon;
    const double c = 2.0 * qAtan2(qSqrt(aa), qSqrt(1.0 - aa));
    return kEarthRadiusKm * c;
}
