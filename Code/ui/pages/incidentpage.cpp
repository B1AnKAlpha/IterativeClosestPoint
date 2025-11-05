#include "ui/pages/incidentpage.h"

#include "services/incidentservice.h"
#include "ui/components/infocard.h"

#include <ElaPushButton.h>
#include <ElaText.h>

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPalette>
#include <QSet>
#include <QSignalBlocker>
#include <QVBoxLayout>

IncidentPage::IncidentPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("事件监控"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);

    auto *title = new ElaText(tr("事件监控"), this);
    title->setTextStyle(ElaTextType::Title);
    title->setTextPixelSize(22);
    layout->addWidget(title);

    auto *subtitle = new ElaText(tr("实时掌握异常告警、处理进度与责任人分派状态"), this);
    subtitle->setTextStyle(ElaTextType::Caption);
    subtitle->setTextPixelSize(13);
    auto captionPalette = subtitle->palette();
    captionPalette.setColor(QPalette::WindowText, captionPalette.color(QPalette::WindowText).lighter(140));
    subtitle->setPalette(captionPalette);
    layout->addWidget(subtitle);

    auto *overviewRow = new QHBoxLayout();
    overviewRow->setSpacing(20);

    m_todayCard = new InfoCard(this);
    m_todayCard->setTitle(tr("今日新增"));
    m_todayCard->setValue(tr("0"));
    m_todayCard->setTrend(tr("0"), true);
    overviewRow->addWidget(m_todayCard);

    m_resolvedCard = new InfoCard(this);
    m_resolvedCard->setTitle(tr("已处理"));
    m_resolvedCard->setValue(tr("0"));
    m_resolvedCard->setTrend(tr("0"), true);
    overviewRow->addWidget(m_resolvedCard);

    m_pendingCard = new InfoCard(this);
    m_pendingCard->setTitle(tr("待确认"));
    m_pendingCard->setValue(tr("0"));
    m_pendingCard->setTrend(tr("0"), false);
    overviewRow->addWidget(m_pendingCard);

    layout->addLayout(overviewRow);

    auto *listTitle = new ElaText(tr("告警明细"), this);
    listTitle->setTextStyle(ElaTextType::Subtitle);
    listTitle->setTextPixelSize(16);
    layout->addWidget(listTitle);

    m_incidentList = new QListWidget(this);
    m_incidentList->setFrameShape(QFrame::NoFrame);
    m_incidentList->setSpacing(6);
    layout->addWidget(m_incidentList);

    auto *ackButton = new ElaPushButton(tr("标记为已处理"), this);
    ackButton->setMinimumHeight(36);
    layout->addWidget(ackButton);

    initializeOverview();

    connect(ackButton, &ElaPushButton::clicked, this, [this]() {
        if (!m_service || !m_incidentList) {
            return;
        }
        const int row = m_incidentList->currentRow();
        if (row < 0 || row >= m_records.size()) {
            return;
        }
        m_service->acknowledgeIncident(m_records.at(row).id);
    });
}

void IncidentPage::setIncidentService(IncidentService *service)
{
    if (m_service == service) {
        return;
    }
    if (m_service) {
        disconnect(m_service, nullptr, this, nullptr);
    }
    m_service = service;
    if (!m_service) {
        return;
    }

    connect(m_service, &IncidentService::incidentFeedReady, this, &IncidentPage::onIncidentFeed);
    connect(m_service, &IncidentService::incidentAdded, this, &IncidentPage::onIncidentAdded);
    connect(m_service, &IncidentService::incidentUpdated, this, &IncidentPage::onIncidentUpdated);

    onIncidentFeed(m_service->incidents());
}

void IncidentPage::refreshIncidents(const QStringList &incidents)
{
    m_incidentList->clear();
    for (const auto &text : incidents) {
        m_incidentList->addItem(text);
    }
}

void IncidentPage::initializeOverview()
{
    if (!m_todayCard || !m_resolvedCard || !m_pendingCard) {
        return;
    }
    m_todayCard->setSubtitle(tr("刷新时间 %1").arg(QDateTime::currentDateTime().toString("HH:mm")));
    m_resolvedCard->setSubtitle(tr("处理团队在线"));
    m_pendingCard->setSubtitle(tr("待处理事件将优先提醒"));
}

void IncidentPage::onIncidentFeed(const QList<IncidentService::IncidentRecord> &records)
{
    m_records = records;
    rebuildList();
    updateCards(m_records);
}

void IncidentPage::onIncidentAdded(const IncidentService::IncidentRecord &record)
{
    m_records.prepend(record);
    rebuildList();
    updateCards(m_records);
}

void IncidentPage::onIncidentUpdated(const IncidentService::IncidentRecord &record)
{
    for (auto &item : m_records) {
        if (item.id == record.id) {
            item = record;
            break;
        }
    }
    rebuildList();
    updateCards(m_records);
}

void IncidentPage::updateCards(const QList<IncidentService::IncidentRecord> &records)
{
    int pending = 0;
    int resolved = 0;
    QSet<QString> todayVehicles;

    const auto todayStart = QDateTime::currentDateTime().date();
    for (const auto &record : records) {
        if (record.timestamp.date() == todayStart) {
            todayVehicles.insert(record.id);
        }
        if (record.status.contains(tr("已处理"))) {
            ++resolved;
        } else {
            ++pending;
        }
    }

    m_todayCard->setValue(QString::number(qMax(0, todayVehicles.size())));
    m_todayCard->setTrend(QStringLiteral("%1").arg(records.size()), true);
    m_resolvedCard->setValue(QString::number(resolved));
    m_resolvedCard->setTrend(resolved >= pending ? tr("稳健") : tr("提升"), resolved >= pending);
    m_pendingCard->setValue(QString::number(pending));
    m_pendingCard->setTrend(pending > resolved ? tr("警惕") : tr("稳定"), pending <= resolved);
    m_pendingCard->setSubtitle(tr("总计 %1 条记录").arg(records.size()));
    m_todayCard->setSubtitle(tr("刷新时间 %1").arg(QDateTime::currentDateTime().toString("HH:mm")));
}

void IncidentPage::rebuildList()
{
    if (!m_incidentList) {
        return;
    }
    const QSignalBlocker blocker(m_incidentList);
    m_incidentList->clear();
    if (m_records.isEmpty()) {
        m_incidentList->addItem(tr("一切正常，暂无新的告警事件"));
        return;
    }
    for (const auto &record : m_records) {
        const auto text = tr("[%1] 车辆 #%2 · %3 · %4")
                               .arg(record.level)
                               .arg(record.vehicleId)
                               .arg(record.message)
                               .arg(record.timestamp.toString("HH:mm"));
        m_incidentList->addItem(text);
    }
    m_incidentList->setCurrentRow(0);
}
