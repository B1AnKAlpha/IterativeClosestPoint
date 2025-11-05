#include "ui/pages/playbackpage.h"

#include <ElaMessageBar.h>
#include <ElaPushButton.h>
#include <ElaText.h>

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPalette>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include "services/trajectoryservice.h"

namespace {
constexpr int kPlaybackIntervalMs = 450;
}

PlaybackPage::PlaybackPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("轨迹回放"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);

    auto *title = new ElaText(tr("轨迹回放"), this);
    title->setTextStyle(ElaTextType::Title);
    title->setTextPixelSize(22);
    layout->addWidget(title);

    auto *subtitle = new ElaText(tr("从历史检索结果中选择轨迹，播放时会实时联动可视化场景"), this);
    subtitle->setTextStyle(ElaTextType::Caption);
    subtitle->setTextPixelSize(13);
    auto captionPalette = subtitle->palette();
    captionPalette.setColor(QPalette::WindowText, captionPalette.color(QPalette::WindowText).lighter(140));
    subtitle->setPalette(captionPalette);
    layout->addWidget(subtitle);

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(24);

    auto *infoColumn = new QVBoxLayout();
    infoColumn->setSpacing(16);

    auto *currentLabel = new ElaText(tr("当前选中"), this);
    currentLabel->setTextStyle(ElaTextType::Subtitle);
    currentLabel->setTextPixelSize(16);
    infoColumn->addWidget(currentLabel);

    m_selectionLabel = new ElaText(tr("等待轨迹数据导入"), this);
    m_selectionLabel->setTextStyle(ElaTextType::Body);
    m_selectionLabel->setWordWrap(true);
    infoColumn->addWidget(m_selectionLabel);

    m_playButton = new ElaPushButton(tr("开始回放"), this);
    m_playButton->setEnabled(false);
    m_playButton->setMinimumHeight(38);
    infoColumn->addWidget(m_playButton);

    m_stopButton = new ElaPushButton(tr("停止"), this);
    m_stopButton->setEnabled(false);
    m_stopButton->setMinimumHeight(36);
    infoColumn->addWidget(m_stopButton);

    infoColumn->addStretch(1);
    contentRow->addLayout(infoColumn, 1);

    m_sessionList = new QListWidget(this);
    m_sessionList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sessionList->setFrameShape(QFrame::NoFrame);
    m_sessionList->setSpacing(4);
    contentRow->addWidget(m_sessionList, 1);

    layout->addLayout(contentRow);
    layout->addStretch(1);

    m_timer = new QTimer(this);
    m_timer->setInterval(kPlaybackIntervalMs);

    connect(m_sessionList, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        updateSelectionInfo(text);
    });

    connect(m_playButton, &ElaPushButton::clicked, this, &PlaybackPage::startPlayback);
    connect(m_stopButton, &ElaPushButton::clicked, this, &PlaybackPage::stopPlayback);
    connect(m_timer, &QTimer::timeout, this, &PlaybackPage::stepPlayback);
}

void PlaybackPage::setTrajectoryService(TrajectoryService *service)
{
    m_service = service;
}

void PlaybackPage::addTrack(const QString &trajectoryId, const QVector<QPointF> &points,
                            const QVector<QDateTime> &timeline)
{
    if (points.size() < 2 || timeline.size() != points.size()) {
        return;
    }

    PlaybackTrack track;
    track.points = points;
    track.timeline = timeline;
    m_tracks.insert(trajectoryId, track);
    if (!m_trackOrder.contains(trajectoryId)) {
        m_trackOrder.prepend(trajectoryId);
    }
    rebuildList();
}

void PlaybackPage::refreshTrackList(const QStringList &ids)
{
    m_trackOrder = ids;
    rebuildList();
}

void PlaybackPage::rebuildList()
{
    const QSignalBlocker blocker(m_sessionList);
    m_sessionList->clear();
    for (const auto &trackId : m_trackOrder) {
        const auto label = tr("车辆 %1 (%2 点)")
                               .arg(trackId)
                               .arg(m_tracks.value(trackId).points.size());
        m_sessionList->addItem(label);
    }
    if (m_sessionList->count() > 0) {
        m_sessionList->setCurrentRow(0);
    } else {
        updateSelectionInfo(QString());
    }
}

void PlaybackPage::updateSelectionInfo(const QString &text)
{
    const bool hasSelection = !text.isEmpty();
    m_playButton->setEnabled(hasSelection);
    m_stopButton->setEnabled(hasSelection && m_timer->isActive());

    if (!hasSelection) {
        m_selectionLabel->setText(tr("等待轨迹数据导入"));
        return;
    }

    const int row = m_sessionList->currentRow();
    if (row < 0 || row >= m_trackOrder.size()) {
        m_selectionLabel->setText(tr("选择一条轨迹以回放"));
        return;
    }

    const auto trajectoryId = m_trackOrder.at(row);
    const auto &track = m_tracks.value(trajectoryId);
    if (track.points.isEmpty() || track.timeline.isEmpty()) {
        m_selectionLabel->setText(tr("轨迹数据为空"));
        return;
    }
    const auto start = track.timeline.first();
    const auto end = track.timeline.last();
    m_selectionLabel->setText(tr("轨迹 %1 · %2 — %3 · %4 点")
                                  .arg(trajectoryId)
                                  .arg(start.toString("HH:mm"))
                                  .arg(end.toString("HH:mm"))
                                  .arg(track.points.size()));
}

void PlaybackPage::startPlayback()
{
    if (m_timer->isActive()) {
        stopPlayback();
    }
    const int row = m_sessionList->currentRow();
    if (row < 0 || row >= m_trackOrder.size()) {
        return;
    }
    const auto trajectoryId = m_trackOrder.at(row);
    auto track = m_tracks.value(trajectoryId);
    if (track.points.size() < 2) {
        ElaMessageBar::warning(ElaMessageBarType::PositionPolicy::TopRight, QString(),
                               tr("轨迹数据不足，无法回放"), 2500, this);
        return;
    }

    track.currentIndex = 0;
    m_activeTrack = trajectoryId;
    m_tracks[trajectoryId] = track;

    emit playbackStarted(trajectoryId);
    m_timer->start();
    m_playButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    updateSelectionInfo(m_sessionList->currentItem() ? m_sessionList->currentItem()->text() : QString());
}

void PlaybackPage::stopPlayback()
{
    if (m_activeTrack.isEmpty()) {
        return;
    }
    m_timer->stop();
    if (m_tracks.contains(m_activeTrack)) {
        auto track = m_tracks.value(m_activeTrack);
        track.currentIndex = 0;
        m_tracks[m_activeTrack] = track;
    }
    emit playbackStopped(m_activeTrack);
    m_playButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_activeTrack.clear();
    updateSelectionInfo(m_sessionList->currentItem() ? m_sessionList->currentItem()->text() : QString());
}

void PlaybackPage::stepPlayback()
{
    if (m_activeTrack.isEmpty()) {
        m_timer->stop();
        return;
    }

    auto track = m_tracks.value(m_activeTrack);
    if (track.currentIndex >= track.points.size()) {
        stopPlayback();
        return;
    }

    const auto index = track.currentIndex;
    emit playbackFrameChanged(m_activeTrack, track.points.at(index), track.timeline.at(index));

    ++track.currentIndex;
    m_tracks[m_activeTrack] = track;

    if (track.currentIndex >= track.points.size()) {
        stopPlayback();
    }
}
