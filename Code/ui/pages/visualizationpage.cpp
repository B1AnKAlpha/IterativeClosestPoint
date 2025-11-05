#include "ui/pages/visualizationpage.h"

#include <ElaText.h>

#include <algorithm>

#include <QDateTime>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineF>
#include <QMap>
#include <QPalette>
#include <QPen>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QBrush>
#include <QColor>
#include <QMarginsF>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QtGlobal>
#include <cmath>

namespace {
constexpr int kCanvasExtent = 480;
constexpr int kScenePadding = 360;
constexpr int kGridStep = 80;
constexpr double kZoomStep = 1.15;
constexpr double kMinZoom = 0.25;
constexpr double kMaxZoom = 5.0;

QPen gridPen()
{
    auto color = QColor(150, 150, 150, 60);
    QPen pen(color);
    pen.setWidthF(1.0);
    pen.setStyle(Qt::DashLine);
    return pen;
}

QPen passivePen()
{
    QPen pen(QColor(170, 170, 170));
    pen.setWidthF(2.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

QPen activePen()
{
    QPen pen(QColor(46, 134, 222));
    pen.setWidthF(3.2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

QPen pointPen(const QColor &color)
{
    QPen pen(color);
    pen.setWidthF(1.2);
    pen.setCosmetic(true);
    return pen;
}
} // namespace

class TrajectoryGraphicsView : public QGraphicsView
{
public:
    explicit TrajectoryGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
        setRenderHint(QPainter::Antialiasing, true);
        setRenderHint(QPainter::SmoothPixmapTransform, true);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorViewCenter);
        setFrameShape(QFrame::NoFrame);
        viewport()->setCursor(Qt::OpenHandCursor);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void resetToBounds(const QRectF &bounds)
    {
        if (bounds.isNull()) {
            return;
        }
        const auto padded = bounds.marginsAdded(QMarginsF(80, 80, 80, 80));
        fitInView(padded, Qt::KeepAspectRatio);
        m_currentZoom = 1.0;
    }

    void focusOnBounds(const QRectF &bounds)
    {
        if (bounds.isNull()) {
            return;
        }
        const auto padded = bounds.marginsAdded(QMarginsF(80, 80, 80, 80));
        fitInView(padded, Qt::KeepAspectRatio);
        m_currentZoom = 1.0;
    }

    void setGridVisible(bool enabled)
    {
        if (m_showGrid == enabled) {
            return;
        }
        m_showGrid = enabled;
        viewport()->update();
    }

    void setSmoothRendering(bool enabled)
    {
        if (m_smoothRendering == enabled) {
            return;
        }
        m_smoothRendering = enabled;
        setRenderHint(QPainter::Antialiasing, enabled);
        setRenderHint(QPainter::SmoothPixmapTransform, enabled);
        viewport()->update();
    }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (!scene()) {
            event->ignore();
            return;
        }
        const auto delta = event->angleDelta().y();
        if (delta == 0) {
            event->ignore();
            return;
        }

        double targetFactor = delta > 0 ? kZoomStep : (1.0 / kZoomStep);
        double newZoom = std::clamp(m_currentZoom * targetFactor, kMinZoom, kMaxZoom);
        targetFactor = newZoom / m_currentZoom;
        if (qFuzzyCompare(targetFactor, 1.0)) {
            event->accept();
            return;
        }
        scale(targetFactor, targetFactor);
        m_currentZoom = newZoom;
        event->accept();
    }

    void drawBackground(QPainter *painter, const QRectF &rect) override
    {
        painter->save();
        painter->fillRect(rect, QColor(248, 250, 254));

        if (m_showGrid) {
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setPen(gridPen());

            const int left = static_cast<int>(std::floor(rect.left() / kGridStep) * kGridStep);
            const int right = static_cast<int>(std::ceil(rect.right() / kGridStep) * kGridStep);
            const int top = static_cast<int>(std::floor(rect.top() / kGridStep) * kGridStep);
            const int bottom = static_cast<int>(std::ceil(rect.bottom() / kGridStep) * kGridStep);

            for (int x = left; x <= right; x += kGridStep) {
                painter->drawLine(QLineF(x, top, x, bottom));
            }
            for (int y = top; y <= bottom; y += kGridStep) {
                painter->drawLine(QLineF(left, y, right, y));
            }
        }

        painter->restore();
    }

private:
    double m_currentZoom{1.0};
    bool m_showGrid{true};
    bool m_smoothRendering{true};
};

VisualizationPage::VisualizationPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("可视化中心"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *title = new ElaText(tr("轨迹可视化"), this);
    title->setTextStyle(ElaTextType::Title);
    title->setTextPixelSize(22);
    layout->addWidget(title);

    auto *subtitle = new ElaText(tr("展示最近检索到的轨迹走势，可通过查询页面快速聚焦"), this);
    subtitle->setTextStyle(ElaTextType::Caption);
    subtitle->setTextPixelSize(13);
    auto captionPalette = subtitle->palette();
    captionPalette.setColor(QPalette::WindowText, captionPalette.color(QPalette::WindowText).lighter(140));
    subtitle->setPalette(captionPalette);
    layout->addWidget(subtitle);

    auto *stack = new QStackedLayout();

    m_viewContainer = new QWidget(this);
    auto *viewLayout = new QVBoxLayout(m_viewContainer);
    viewLayout->setContentsMargins(12, 12, 12, 12);
    viewLayout->setSpacing(0);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-kCanvasExtent - kScenePadding,
                          -kCanvasExtent - kScenePadding,
                          (kCanvasExtent + kScenePadding) * 2,
                          (kCanvasExtent + kScenePadding) * 2);

    m_view = new TrajectoryGraphicsView(m_scene, m_viewContainer);
    auto *trajectoryView = static_cast<TrajectoryGraphicsView *>(m_view);
    trajectoryView->setGridVisible(m_showGridEnabled);
    trajectoryView->setSmoothRendering(m_smoothRenderingEnabled);
    viewLayout->addWidget(m_view);

    stack->addWidget(m_viewContainer);

    m_placeholder = new ElaText(tr("等待轨迹数据…"), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setTextStyle(ElaTextType::Body);
    m_placeholder->setTextPixelSize(14);
    stack->addWidget(m_placeholder);

    layout->addLayout(stack, 1);

    updatePlaceholderVisibility();
}

void VisualizationPage::showTrajectory(const QString &trajectoryId, const QVector<QPointF> &points)
{
    if (!m_scene || points.size() < 2) {
        return;
    }

    if (m_paths.contains(trajectoryId)) {
        const auto visual = m_paths.take(trajectoryId);
        if (visual.path) {
            m_scene->removeItem(visual.path);
            delete visual.path;
        }
        if (visual.start) {
            m_scene->removeItem(visual.start);
            delete visual.start;
        }
        if (visual.end) {
            m_scene->removeItem(visual.end);
            delete visual.end;
        }
    }

    double minX = points.first().x();
    double maxX = minX;
    double minY = points.first().y();
    double maxY = minY;
    for (const auto &pt : points) {
        minX = std::min(minX, static_cast<double>(pt.x()));
        maxX = std::max(maxX, static_cast<double>(pt.x()));
        minY = std::min(minY, static_cast<double>(pt.y()));
        maxY = std::max(maxY, static_cast<double>(pt.y()));
    }
    const double width = std::max(1.0, maxX - minX);
    const double height = std::max(1.0, maxY - minY);

    auto normalize = [&](const QPointF &pt) {
        const double normX = (pt.x() - minX) / width;
        const double normY = (pt.y() - minY) / height;
        const double sceneX = -kCanvasExtent + normX * (kCanvasExtent * 2);
        const double sceneY = kCanvasExtent - normY * (kCanvasExtent * 2);
        return QPointF(sceneX, sceneY);
    };

    QPainterPath painterPath(normalize(points.first()));
    for (int i = 1; i < points.size(); ++i) {
        painterPath.lineTo(normalize(points.at(i)));
    }

    auto *pathItem = new QGraphicsPathItem(painterPath);
    pathItem->setPen(passivePen());
    pathItem->setZValue(1.0);
    m_scene->addItem(pathItem);

    const QPointF startPos = normalize(points.first());
    const QPointF endPos = normalize(points.last());
    const qreal markerRadius = 6.0;

    auto *startItem = m_scene->addEllipse(QRectF(startPos.x() - markerRadius, startPos.y() - markerRadius,
                                                 markerRadius * 2, markerRadius * 2),
                                          pointPen(QColor(46, 204, 113)), QBrush(QColor(46, 204, 113, 160)));
    startItem->setZValue(2.0);

    auto *endItem = m_scene->addEllipse(QRectF(endPos.x() - markerRadius, endPos.y() - markerRadius,
                                               markerRadius * 2, markerRadius * 2),
                                        pointPen(QColor(231, 76, 60)), QBrush(QColor(231, 76, 60, 180)));
    endItem->setZValue(2.0);

    TrajectoryVisual visual;
    visual.path = pathItem;
    visual.start = startItem;
    visual.end = endItem;
    visual.minX = minX;
    visual.maxX = maxX;
    visual.minY = minY;
    visual.maxY = maxY;

    m_paths.insert(trajectoryId, visual);
    m_activeId = trajectoryId;
    highlightTrajectory(trajectoryId);
    updatePlaceholderVisibility();

    static_cast<TrajectoryGraphicsView *>(m_view)->resetToBounds(painterPath.boundingRect());
}

void VisualizationPage::focusOnTrajectory(const QString &trajectoryId)
{
    if (!m_scene) {
        return;
    }
    if (!m_paths.contains(trajectoryId)) {
        if (m_placeholder) {
            m_placeholder->setText(tr("暂无轨迹 %1 的可视化数据").arg(trajectoryId));
            if (m_viewContainer) {
                m_viewContainer->setVisible(false);
            }
            m_placeholder->setVisible(true);
        }
        return;
    }
    m_activeId = trajectoryId;
    highlightTrajectory(trajectoryId);
    if (m_viewContainer) {
        m_viewContainer->setVisible(true);
    }
    if (m_placeholder) {
        m_placeholder->setVisible(false);
    }
    if (auto *pathItem = m_paths.value(trajectoryId).path) {
        static_cast<TrajectoryGraphicsView *>(m_view)->focusOnBounds(pathItem->boundingRect());
    }
}

void VisualizationPage::updatePlaceholderVisibility()
{
    if (!m_viewContainer || !m_placeholder) {
        return;
    }
    const bool hasData = !m_paths.isEmpty();
    m_viewContainer->setVisible(hasData);
    if (!hasData && m_placeholder->text().trimmed().isEmpty()) {
        m_placeholder->setText(tr("等待轨迹数据…"));
    }
    m_placeholder->setVisible(!hasData);
}

void VisualizationPage::setShowGridEnabled(bool enabled)
{
    if (m_showGridEnabled == enabled) {
        return;
    }
    m_showGridEnabled = enabled;
    if (m_view) {
        auto *view = static_cast<TrajectoryGraphicsView *>(m_view);
        view->setGridVisible(enabled);
    }
}

void VisualizationPage::setSmoothRenderingEnabled(bool enabled)
{
    if (m_smoothRenderingEnabled == enabled) {
        return;
    }
    m_smoothRenderingEnabled = enabled;
    if (m_view) {
        auto *view = static_cast<TrajectoryGraphicsView *>(m_view);
        view->setSmoothRendering(enabled);
    }
}

void VisualizationPage::highlightTrajectory(const QString &trajectoryId)
{
    for (auto it = m_paths.begin(); it != m_paths.end(); ++it) {
        if (!it.value().path) {
            continue;
        }
        it.value().path->setPen(it.key() == trajectoryId ? activePen() : passivePen());
        if (it.value().start) {
            it.value().start->setOpacity(it.key() == trajectoryId ? 1.0 : 0.4);
        }
        if (it.value().end) {
            it.value().end->setOpacity(it.key() == trajectoryId ? 1.0 : 0.4);
        }
    }
}

void VisualizationPage::startPlayback(const QString &trajectoryId)
{
    if (!m_scene) {
        return;
    }

    focusOnTrajectory(trajectoryId);

    if (!m_playbackMarker) {
        constexpr qreal radius = 7.0;
        m_playbackMarker = m_scene->addEllipse(-radius, -radius, radius * 2, radius * 2,
                                               pointPen(QColor(241, 196, 15)),
                                               QBrush(QColor(241, 196, 15, 200)));
        m_playbackMarker->setZValue(3.0);
    }
    if (!m_playbackLabel) {
        m_playbackLabel = m_scene->addSimpleText(QString());
        m_playbackLabel->setBrush(QBrush(QColor(50, 50, 50)));
        m_playbackLabel->setZValue(3.0);
    }

    m_playbackMarker->setVisible(true);
    m_playbackLabel->setVisible(true);
    highlightTrajectory(trajectoryId);
}

void VisualizationPage::stopPlayback()
{
    if (m_playbackMarker) {
        m_playbackMarker->setVisible(false);
    }
    if (m_playbackLabel) {
        m_playbackLabel->setVisible(false);
    }
}

void VisualizationPage::updatePlaybackFrame(const QString &trajectoryId, const QPointF &point,
                                            const QDateTime &timestamp)
{
    if (!m_playbackMarker) {
        startPlayback(trajectoryId);
    }
    const auto scenePoint = mapToScene(trajectoryId, point);
    if (m_playbackMarker) {
        const QRectF rect = m_playbackMarker->rect();
        m_playbackMarker->setPos(scenePoint.x() - rect.width() / 2.0,
                                 scenePoint.y() - rect.height() / 2.0);
    }
    if (m_playbackLabel) {
        m_playbackLabel->setText(timestamp.toString("HH:mm:ss"));
        m_playbackLabel->setPos(scenePoint + QPointF(8.0, -16.0));
    }
}

QPointF VisualizationPage::mapToScene(const QString &trajectoryId, const QPointF &rawPoint) const
{
    if (!m_paths.contains(trajectoryId)) {
        return rawPoint;
    }
    const auto visual = m_paths.value(trajectoryId);
    const double minX = visual.minX;
    const double maxX = visual.maxX;
    const double minY = visual.minY;
    const double maxY = visual.maxY;

    const double width = std::max(1.0, maxX - minX);
    const double height = std::max(1.0, maxY - minY);
    const double normX = (rawPoint.x() - minX) / width;
    const double normY = (rawPoint.y() - minY) / height;
    const double sceneX = -kCanvasExtent + normX * (kCanvasExtent * 2);
    const double sceneY = kCanvasExtent - normY * (kCanvasExtent * 2);
    return QPointF(sceneX, sceneY);
}
