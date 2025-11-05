#include "ui/pages/analyticspage.h"

#include "ui/components/infocard.h"

#include <QGridLayout>
#include <QVBoxLayout>

AnalyticsPage::AnalyticsPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("分析实验室"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(24);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(24);

    m_throughputCard = new InfoCard(this);
    m_throughputCard->setTitle(tr("处理吞吐"));
    m_throughputCard->setValue(tr("2.6k/s"));
    m_throughputCard->setTrend(tr("+8.0%"), true);

    m_latencyCard = new InfoCard(this);
    m_latencyCard->setTitle(tr("平均延迟"));
    m_latencyCard->setValue(tr("180ms"));
    m_latencyCard->setTrend(tr("-5.0%"), false);

    grid->addWidget(m_throughputCard, 0, 0);
    grid->addWidget(m_latencyCard, 0, 1);

    layout->addLayout(grid);
}

void AnalyticsPage::updateMetrics(const QVariantMap &metrics)
{
    if (m_throughputCard) {
        const auto throughput = metrics.value(QStringLiteral("throughput"));
        if (!throughput.isNull()) {
            m_throughputCard->setValue(throughput.toString());
        }
    }
    if (m_latencyCard) {
        const auto latency = metrics.value(QStringLiteral("latency"));
        if (!latency.isNull()) {
            m_latencyCard->setValue(latency.toString());
        }
    }
}
