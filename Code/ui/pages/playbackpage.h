#ifndef UI_PAGES_PLAYBACKPAGE_H
#define UI_PAGES_PLAYBACKPAGE_H

#include <QWidget>

#include <QDateTime>
#include <QHash>
#include <QPointF>
#include <QStringList>
#include <QVector>

class QListWidget;
class ElaPushButton;
class ElaText;
class QTimer;
class TrajectoryService;

class PlaybackPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlaybackPage(QWidget *parent = nullptr);

    void setTrajectoryService(TrajectoryService *service);
    void addTrack(const QString &trajectoryId, const QVector<QPointF> &points,
                  const QVector<QDateTime> &timeline);
    void refreshTrackList(const QStringList &ids);

signals:
    void playbackFrameChanged(const QString &trajectoryId, const QPointF &position,
                               const QDateTime &timestamp);
    void playbackStarted(const QString &trajectoryId);
    void playbackStopped(const QString &trajectoryId);

private:
    void rebuildList();
    void updateSelectionInfo(const QString &text);
    void startPlayback();
    void stopPlayback();
    void stepPlayback();

    QListWidget *m_sessionList{nullptr};
    ElaPushButton *m_playButton{nullptr};
    ElaText *m_selectionLabel{nullptr};
    ElaPushButton *m_stopButton{nullptr};
    QTimer *m_timer{nullptr};
    TrajectoryService *m_service{nullptr};

    struct PlaybackTrack {
        QVector<QPointF> points;
        QVector<QDateTime> timeline;
        int currentIndex{0};
    };

    QHash<QString, PlaybackTrack> m_tracks;
    QStringList m_trackOrder;
    QString m_activeTrack;
};

#endif // UI_PAGES_PLAYBACKPAGE_H
