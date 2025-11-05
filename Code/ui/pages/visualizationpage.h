#ifndef UI_PAGES_VISUALIZATIONPAGE_H
#define UI_PAGES_VISUALIZATIONPAGE_H

#include <QWidget>
#include <QString>
#include <QMap>
#include <QVector>
#include <QPointF>
#include <QDateTime>

class QGraphicsView;
class QGraphicsScene;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsSimpleTextItem;
class ElaText;

class VisualizationPage : public QWidget
{
    Q_OBJECT

public:
    explicit VisualizationPage(QWidget *parent = nullptr);

    void showTrajectory(const QString &trajectoryId, const QVector<QPointF> &points);
    void focusOnTrajectory(const QString &trajectoryId);
    void startPlayback(const QString &trajectoryId);
    void stopPlayback();
    void updatePlaybackFrame(const QString &trajectoryId, const QPointF &point, const QDateTime &timestamp);
    void setShowGridEnabled(bool enabled);
    void setSmoothRenderingEnabled(bool enabled);

private:
    void updatePlaceholderVisibility();
    void highlightTrajectory(const QString &trajectoryId);

    struct TrajectoryVisual {
        QGraphicsPathItem *path{nullptr};
        QGraphicsEllipseItem *start{nullptr};
        QGraphicsEllipseItem *end{nullptr};
        double minX{0.0};
        double maxX{0.0};
        double minY{0.0};
        double maxY{0.0};
    };

    QPointF mapToScene(const QString &trajectoryId, const QPointF &rawPoint) const;

    QGraphicsEllipseItem *m_playbackMarker{nullptr};
    QGraphicsSimpleTextItem *m_playbackLabel{nullptr};

    QGraphicsView *m_view{nullptr};
    QGraphicsScene *m_scene{nullptr};
    QWidget *m_viewContainer{nullptr};
    ElaText *m_placeholder{nullptr};
    QMap<QString, TrajectoryVisual> m_paths;
    QString m_activeId;
    bool m_showGridEnabled{true};
    bool m_smoothRenderingEnabled{true};
};

#endif // UI_PAGES_VISUALIZATIONPAGE_H
