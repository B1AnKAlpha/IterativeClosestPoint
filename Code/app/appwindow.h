#ifndef APP_APPWINDOW_H
#define APP_APPWINDOW_H

#include <ElaWindow.h>

class DashboardPage;
class QueryWorkbenchPage;
class VisualizationPage;
class DataManagerPage;
class SettingsPage;
class PlaybackPage;
class IncidentPage;
class ReportPage;
class ElaContentDialog;
class QueryManager;
class TrajectoryService;
class StatisticService;
class IncidentService;
class ReportService;
class SettingsService;
class QuadTreeRepository;
struct QueryParameters;
struct QueryRecord;

class AppWindow : public ElaWindow
{
    Q_OBJECT

public:
    explicit AppWindow(QWidget *parent = nullptr);
    ~AppWindow() override;

private:
    void initializeWindow();
    void applyAppBarBrandingOverride();
    void setupNavigation();
    void setupStatusBar();
    void connectSignals();
    void setupServices();
    void refreshMetrics();
    QueryRecord buildRecord(const QueryParameters &parameters, int resultCount) const;
    void restoreLastSession();
    void persistCurrentPage() const;

    DashboardPage *m_dashboardPage{nullptr};
    QueryWorkbenchPage *m_queryPage{nullptr};
    VisualizationPage *m_visualizationPage{nullptr};
    DataManagerPage *m_dataManagerPage{nullptr};
    SettingsPage *m_settingsPage{nullptr};
    PlaybackPage *m_playbackPage{nullptr};
    IncidentPage *m_incidentPage{nullptr};
    ReportPage *m_reportPage{nullptr};

    QString m_dashboardKey;
    QString m_queryKey;
    QString m_visualizationKey;
    QString m_dataManagerKey;
    QString m_settingsKey;
    QString m_playbackKey;
    QString m_incidentKey;
    QString m_reportKey;

    ElaContentDialog *m_closeDialog{nullptr};
    QueryManager *m_queryManager{nullptr};
    TrajectoryService *m_trajectoryService{nullptr};
    StatisticService *m_statisticService{nullptr};
    IncidentService *m_incidentService{nullptr};
    ReportService *m_reportService{nullptr};
    SettingsService *m_settingsService{nullptr};
    QStringList m_lastLoadedFiles;
    int m_lastDepth{8};
    QString m_currentPageKey;
};

#endif // APP_APPWINDOW_H
