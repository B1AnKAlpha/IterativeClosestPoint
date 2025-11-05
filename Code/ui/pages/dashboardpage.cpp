#include "ui/pages/dashboardpage.h"

#include "ui/components/infocard.h"

#include <ElaMessageBar.h>
#include <ElaPushButton.h>
#include <ElaText.h>
#include <QDateTime>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSizePolicy>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("仪表盘"));
    buildLayout();
}

void DashboardPage::buildLayout()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(24);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(20);
    grid->setVerticalSpacing(20);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);

    m_activeCard = new InfoCard(this);
    m_activeCard->setTitle(tr("活跃轨迹"));
    m_activeCard->setValue(tr("123"));
    m_activeCard->setTrend(tr("+12.5%"), true);

    m_alertCard = new InfoCard(this);
    m_alertCard->setTitle(tr("预警事件"));
    m_alertCard->setValue(tr("4"));
    m_alertCard->setTrend(tr("-3.0%"), false);

    m_volumeCard = new InfoCard(this);
    m_volumeCard->setTitle(tr("数据量"));
    m_volumeCard->setValue(tr("1.8M"));
    m_volumeCard->setTrend(tr("0%"), true);

    grid->addWidget(m_activeCard, 0, 0);
    grid->addWidget(m_alertCard, 0, 1);
    grid->addWidget(m_volumeCard, 0, 2);

    layout->addLayout(grid);

    auto *analyticsLabel = new ElaText(tr("运行监控"), this);
    analyticsLabel->setTextStyle(ElaTextType::Subtitle);
    analyticsLabel->setTextPixelSize(16);
    layout->addWidget(analyticsLabel);

    auto *analyticsRow = new QHBoxLayout();
    analyticsRow->setSpacing(20);

    m_throughputCard = new InfoCard(this);
    m_throughputCard->setTitle(tr("处理吞吐"));
    m_throughputCard->setValue(tr("2.6k/s"));
    m_throughputCard->setTrend(tr("+8.0%"), true);

    m_latencyCard = new InfoCard(this);
    m_latencyCard->setTitle(tr("平均延迟"));
    m_latencyCard->setValue(tr("180ms"));
    m_latencyCard->setTrend(tr("-5.0%"), false);

    analyticsRow->addWidget(m_throughputCard);
    analyticsRow->addWidget(m_latencyCard);
    layout->addLayout(analyticsRow);

    auto *cta = new ElaPushButton(tr("打开查询工作台"), this);
    cta->setMinimumHeight(40);
    cta->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(cta);
    layout->addStretch(1);
    connect(cta, &ElaPushButton::clicked, this, [this]() {
        emit requestNavigation(QStringLiteral("query"));
    });
}

void DashboardPage::triggerTreeBuild()
{
    if (!m_activeCard) {
        return;
    }
    m_activeCard->setSubtitle(tr("四叉树重建完成：%1")
                                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    ElaMessageBar::information(ElaMessageBarType::PositionPolicy::TopRight, QString(), tr("四叉树结构已更新"), 3000, this);
}

void DashboardPage::updateMetrics(const QVariantMap &metrics)
{
    if (m_activeCard && metrics.contains(QStringLiteral("activeTrajectories"))) {
        m_activeCard->setValue(metrics.value(QStringLiteral("activeTrajectories")).toString());
    }
    if (m_alertCard && metrics.contains(QStringLiteral("alerts"))) {
        m_alertCard->setValue(metrics.value(QStringLiteral("alerts")).toString());
    }
    if (m_volumeCard && metrics.contains(QStringLiteral("dataVolume"))) {
        m_volumeCard->setValue(metrics.value(QStringLiteral("dataVolume")).toString());
    }
    if (m_throughputCard && metrics.contains(QStringLiteral("throughput"))) {
        m_throughputCard->setValue(metrics.value(QStringLiteral("throughput")).toString());
    }
    if (m_latencyCard && metrics.contains(QStringLiteral("latency"))) {
        m_latencyCard->setValue(metrics.value(QStringLiteral("latency")).toString());
    }
}
