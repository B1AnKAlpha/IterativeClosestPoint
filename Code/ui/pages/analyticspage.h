#ifndef UI_PAGES_ANALYTICSPAGE_H
#define UI_PAGES_ANALYTICSPAGE_H

#include <QWidget>

#include <QVariantMap>

class AnalyticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AnalyticsPage(QWidget *parent = nullptr);

    void updateMetrics(const QVariantMap &metrics);

private:
    class InfoCard *m_throughputCard{nullptr};
    class InfoCard *m_latencyCard{nullptr};
};

#endif // UI_PAGES_ANALYTICSPAGE_H
