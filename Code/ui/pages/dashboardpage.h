#ifndef UI_PAGES_DASHBOARDPAGE_H
#define UI_PAGES_DASHBOARDPAGE_H

#include <QWidget>

#include <QVariantMap>

class InfoCard;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

signals:
    void requestNavigation(const QString &pageKey);

public slots:
    void triggerTreeBuild();
    void updateMetrics(const QVariantMap &metrics);

private:
    void buildLayout();

    InfoCard *m_activeCard{nullptr};
    InfoCard *m_alertCard{nullptr};
    InfoCard *m_volumeCard{nullptr};
    InfoCard *m_throughputCard{nullptr};
    InfoCard *m_latencyCard{nullptr};
};

#endif // UI_PAGES_DASHBOARDPAGE_H
