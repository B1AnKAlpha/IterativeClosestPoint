#include "appwindow.h"

#include <QDateTime>
#include <QIcon>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QPointF>
#include <QPixmap>
#include <QScreen>
#include <QTextStream>
#include <QTimer>
#include <QVariantMap>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>

#include "ElaContentDialog.h"
#include "ElaMenu.h"
#include "ElaMessageBar.h"
#include "ElaStatusBar.h"
#include "ElaText.h"
#include "ElaTheme.h"
#include "common/queryparameters.h"
#include "querymanager.h"
#include "services/statisticservice.h"
#include "services/trajectoryservice.h"
#include "services/incidentservice.h"
#include "services/reportservice.h"
#include "services/settingsservice.h"
#include "services/quadtreerepository.h"
#include "ui/pages/dashboardpage.h"
#include "ui/pages/datamanagerpage.h"
#include "ui/pages/incidentpage.h"
#include "ui/pages/playbackpage.h"
#include "ui/pages/queryworkbenchpage.h"
#include "ui/pages/settingspage.h"
#include "ui/pages/reportpage.h"
#include "ui/pages/visualizationpage.h"

namespace {
ElaText *createStatusText(const QString &text) {
    auto *label = new ElaText(text);
    label->setTextStyle(ElaTextType::Caption);
    label->setTextPixelSize(12);
    return label;
}
} // namespace

AppWindow::AppWindow(QWidget *parent)
    : ElaWindow(parent)
{
    initializeWindow();
    setupServices();
    setupNavigation();
    setupStatusBar();
    connectSignals();
    restoreLastSession();
    refreshMetrics();
}

AppWindow::~AppWindow() = default;

void AppWindow::initializeWindow()
{
    resize(1340, 820);
    setWindowTitle(tr("基于四叉树的行车轨迹管理系统"));
    const auto logoResourcePath = QStringLiteral(":/new/prefix1/quadtracer_logo.png");
    setWindowIcon(QIcon(logoResourcePath));
    setUserInfoCardPixmap(QPixmap(logoResourcePath));
    setUserInfoCardTitle(tr("基于四叉树的行车轨迹管理系统"));
    setUserInfoCardSubTitle(tr("一站式智能交通轨迹分析平台"));
    setNavigationBarDisplayMode(ElaNavigationType::NavigationDisplayMode::Maximal);
    setNavigationBarWidth(320);
    setStackSwitchMode(ElaWindowType::StackSwitchMode::Scale);
    setIsDefaultClosed(false);
    setIsCentralStackedWidgetTransparent(true);

    // 关闭确认对话框
    m_closeDialog = new ElaContentDialog(this);
    auto *dialogContent = new QWidget(m_closeDialog);
    auto *dialogLayout = new QVBoxLayout(dialogContent);
    dialogLayout->setContentsMargins(28, 28, 28, 28);
    dialogLayout->setSpacing(16);
    dialogContent->setMinimumWidth(360);

    auto *dialogTitle = new ElaText(tr("退出确认"), dialogContent);
    dialogTitle->setTextStyle(ElaTextType::Title);
    dialogTitle->setTextPixelSize(20);

    auto *dialogMessage = new ElaText(tr("确定要退出基于四叉树的行车轨迹管理系统吗？"), dialogContent);
    dialogMessage->setTextStyle(ElaTextType::Body);
    dialogMessage->setTextPixelSize(14);
    dialogMessage->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    dialogMessage->setWordWrap(true);

    dialogLayout->addWidget(dialogTitle);
    dialogLayout->addWidget(dialogMessage);

    m_closeDialog->setCentralWidget(dialogContent);
    m_closeDialog->setLeftButtonText(tr("取消"));
    m_closeDialog->setMiddleButtonText(tr("最小化"));
    m_closeDialog->setRightButtonText(tr("退出"));
    connect(m_closeDialog, &ElaContentDialog::leftButtonClicked, m_closeDialog, &ElaContentDialog::close);
    connect(m_closeDialog, &ElaContentDialog::rightButtonClicked, this, &AppWindow::closeWindow);
    connect(m_closeDialog, &ElaContentDialog::middleButtonClicked, this, [this]() {
        m_closeDialog->close();
        showMinimized();
    });
    connect(this, &ElaWindow::closeButtonClicked, this, [this]() {
        m_closeDialog->exec();
    });

    // 自定义 AppBar 菜单
    auto *menu = new ElaMenu(this);
    menu->addAction(tr("返回仪表盘"), this, [this]() { navigation(m_dashboardKey); });
    menu->addAction(tr("切换主题"), this, []() {
        auto mode = eTheme->getThemeMode();
        eTheme->setThemeMode(mode == ElaThemeType::Light ? ElaThemeType::Dark : ElaThemeType::Light);
    });
    menu->addAction(tr("紧凑模式"), this, [this]() {
        setNavigationBarDisplayMode(ElaNavigationType::NavigationDisplayMode::Compact);
    });
    menu->addAction(tr("展开模式"), this, [this]() {
        setNavigationBarDisplayMode(ElaNavigationType::NavigationDisplayMode::Maximal);
    });
    setCustomMenu(menu);
    applyAppBarBrandingOverride();
}

void AppWindow::applyAppBarBrandingOverride()
{
    const auto hideBranding = [this]() {
        if (auto *appBar = findChild<QWidget*>(QStringLiteral("ElaAppBar"))) {
            if (auto *iconLabel = appBar->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly)) {
                iconLabel->setVisible(false);
            }
            if (auto *titleLabel = appBar->findChild<ElaText*>(QString(), Qt::FindDirectChildrenOnly)) {
                titleLabel->setVisible(false);
            }
        }
    };

    hideBranding();

    connect(this, &QWidget::windowTitleChanged, this, [hideBranding](const QString &) {
        hideBranding();
    });
    connect(this, &QWidget::windowIconChanged, this, [hideBranding](const QIcon &) {
        hideBranding();
    });
}

void AppWindow::setupNavigation()
{
    // 创建页面实例
    m_dashboardPage = new DashboardPage(this);
    m_queryPage = new QueryWorkbenchPage(this);
    m_visualizationPage = new VisualizationPage(this);
    m_playbackPage = new PlaybackPage(this);
    m_incidentPage = new IncidentPage(this);
    m_reportPage = new ReportPage(this);
    m_dataManagerPage = new DataManagerPage(this);
    m_settingsPage = new SettingsPage(this);

    if (m_settingsPage && m_settingsService) {
        m_settingsPage->setSettingsService(m_settingsService);
    }

    if (m_reportPage && m_reportService) {
        m_reportPage->setReportService(m_reportService);
    }

    if (m_playbackPage) {
        m_playbackPage->setTrajectoryService(m_trajectoryService);
    }
    if (m_incidentPage) {
        m_incidentPage->setIncidentService(m_incidentService);
    }

    if (m_visualizationPage && m_settingsService) {
        const auto settings = m_settingsService->settings();
        m_visualizationPage->setShowGridEnabled(settings.showMapGrid);
        m_visualizationPage->setSmoothRenderingEnabled(settings.smoothAnimation);
    }

    if (addPageNode(tr("仪表盘"), m_dashboardPage, ElaIconType::ChartMixed) == ElaNavigationType::Success) {
        m_dashboardKey = m_dashboardPage->property("ElaPageKey").toString();
    }

    if (addPageNode(tr("查询工作台"), m_queryPage, ElaIconType::MagnifyingGlassPlus) == ElaNavigationType::Success) {
        m_queryKey = m_queryPage->property("ElaPageKey").toString();
    }

    if (addPageNode(tr("可视化中心"), m_visualizationPage, ElaIconType::MapLocationDot) == ElaNavigationType::Success) {
        m_visualizationKey = m_visualizationPage->property("ElaPageKey").toString();
    }

    if (addPageNode(tr("轨迹回放"), m_playbackPage, ElaIconType::CirclePlay) == ElaNavigationType::Success) {
        m_playbackKey = m_playbackPage->property("ElaPageKey").toString();
    }

    if (addPageNode(tr("事件监控"), m_incidentPage, ElaIconType::ShieldCheck) == ElaNavigationType::Success) {
        m_incidentKey = m_incidentPage->property("ElaPageKey").toString();
    }

    if (addPageNode(tr("报表中心"), m_reportPage, ElaIconType::FileChartPie) == ElaNavigationType::Success) {
        m_reportKey = m_reportPage->property("ElaPageKey").toString();
    }

    if (addPageNode(tr("数据管理"), m_dataManagerPage, ElaIconType::Database) == ElaNavigationType::Success) {
        m_dataManagerKey = m_dataManagerPage->property("ElaPageKey").toString();
    }

    addFooterNode(tr("设置"), m_settingsPage, m_settingsKey, 0, ElaIconType::GearComplex);
    if (m_settingsKey.isEmpty()) {
        m_settingsKey = m_settingsPage->property("ElaPageKey").toString();
    }

    if (m_dataManagerPage && m_queryManager) {
        m_dataManagerPage->setHistory(m_queryManager->records());
    }
}

void AppWindow::setupStatusBar()
{
    auto *statusBar = new ElaStatusBar(this);
    statusBar->setContentsMargins(16, 0, 16, 0);

    auto *statusText = createStatusText(tr("就绪"));
    statusBar->addWidget(statusText);

    auto *timeText = createStatusText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    statusBar->addPermanentWidget(timeText);

    setStatusBar(statusBar);

    // 定时更新时间文本
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [timeText]() {
        timeText->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    });
    timer->start(60000);
}

void AppWindow::connectSignals()
{
    connect(this, &ElaWindow::userInfoCardClicked, this, [this]() {
        if (!m_dashboardKey.isEmpty()) {
            navigation(m_dashboardKey);
        }
    });

    connect(this, &ElaWindow::navigationNodeClicked, this, [this](ElaNavigationType::NavigationNodeType type, const QString &nodeKey) {
        if (type == ElaNavigationType::PageNode) {
            m_currentPageKey = nodeKey;
            persistCurrentPage();
        }
    });

    connect(m_dashboardPage, &DashboardPage::requestNavigation, this, [this](const QString &target) {
        if (target == QLatin1String("query")) {
            if (!m_queryKey.isEmpty()) {
                navigation(m_queryKey);
            }
        } else if (!target.isEmpty()) {
            navigation(target);
        }
    });

    connect(m_queryPage, &QueryWorkbenchPage::requestVisualization, this, [this](const QString &trajectoryId) {
        if (!m_visualizationKey.isEmpty()) {
            navigation(m_visualizationKey);
        }
        m_visualizationPage->focusOnTrajectory(trajectoryId);
    });

    connect(m_dataManagerPage, &DataManagerPage::requestRebuildTree, this, [this]() {
        if (!m_trajectoryService || m_lastLoadedFiles.isEmpty()) {
            ElaMessageBar::warning(ElaMessageBarType::PositionPolicy::TopRight, QString(), tr("暂无可重建的数据源"), 3000, this);
            return;
        }
        if (m_trajectoryService->loadFromFiles(m_lastLoadedFiles, m_lastDepth)) {
            ElaMessageBar::information(ElaMessageBarType::PositionPolicy::TopRight, QString(), tr("四叉树已重新构建"), 3000, this);
        }
    });

    connect(m_dataManagerPage, &DataManagerPage::importRequested, this, [this]() {
        if (!m_trajectoryService) {
            return;
        }
        const QString directory = QFileDialog::getExistingDirectory(this, tr("选择轨迹数据目录"));
        if (directory.isEmpty()) {
            return;
        }
        if (!m_trajectoryService->loadFromDirectory(directory)) {
            return;
        }
        const auto stats = m_trajectoryService->repositoryStatistics();
        m_lastLoadedFiles = m_trajectoryService->loadedFiles();
    m_lastDepth = stats.depth > 0 ? stats.depth : m_lastDepth;
        ElaMessageBar::success(ElaMessageBarType::PositionPolicy::TopRight, QString(), tr("成功导入 %1 个文件，记录 %2 条").arg(stats.fileCount).arg(stats.recordCount), 4000, this);
    });

    connect(m_dataManagerPage, &DataManagerPage::exportRequested, this, [this]() {
        if (!m_queryManager) {
            return;
        }
        const QString filePath = QFileDialog::getSaveFileName(this, tr("导出查询记录"), QString(), tr("CSV 文件 (*.csv)"));
        if (filePath.isEmpty()) {
            return;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ElaMessageBar::warning(ElaMessageBarType::PositionPolicy::TopRight, QString(), tr("无法写入导出文件"), 3000, this);
            return;
        }
        QTextStream out(&file);
        out << "type,vehicle,lonMin,lonMax,latMin,latMax,start,end,resultCount,timestamp\n";
        for (const auto &record : m_queryManager->records()) {
            out << queryTypeToString(record.type) << ','
                << record.parameters.value("vehicle").toString() << ','
                << record.parameters.value("lonMin").toString() << ','
                << record.parameters.value("lonMax").toString() << ','
                << record.parameters.value("latMin").toString() << ','
                << record.parameters.value("latMax").toString() << ','
                << record.parameters.value("start").toDateTime().toString(Qt::ISODate) << ','
                << record.parameters.value("end").toDateTime().toString(Qt::ISODate) << ','
                << record.resultCount << ','
                << record.timestamp.toString(Qt::ISODate) << '\n';
        }
        file.close();
        ElaMessageBar::success(ElaMessageBarType::PositionPolicy::TopRight, QString(), tr("查询记录已导出"), 3000, this);
    });

    connect(m_settingsPage, &SettingsPage::themeModeChanged, this, [](ElaThemeType::ThemeMode type) {
        eTheme->setThemeMode(type);
    });

    connect(m_queryPage, &QueryWorkbenchPage::queryTriggered,
            m_trajectoryService, &TrajectoryService::runQuery);

    connect(m_trajectoryService, &TrajectoryService::queryCompleted, this,
            [this](const QueryParameters &parameters, int resultCount) {
                const auto record = buildRecord(parameters, resultCount);
                m_queryManager->addRecord(record);
                if (m_queryPage && resultCount > 0) {
                    const QString vehicleLabel = parameters.vehicleId.isEmpty()
                                                     ? tr("区域检索")
                                                     : tr("车辆 %1").arg(parameters.vehicleId);
                    const QString summary = tr("%1 · %2 条结果").arg(vehicleLabel).arg(resultCount);
                    m_queryPage->appendResult(summary);
                }
                ElaMessageBar::information(ElaMessageBarType::PositionPolicy::TopRight, QString(),
                                           tr("查询完成，返回 %1 条记录").arg(resultCount), 3000, this);
            });

    connect(m_trajectoryService, &TrajectoryService::queryFailed, this,
            [this](const QString &message) {
                if (m_queryPage) {
                    m_queryPage->appendResult(message);
                }
                ElaMessageBar::warning(ElaMessageBarType::PositionPolicy::TopRight, QString(), message, 4000, this);
            });

    connect(m_trajectoryService, &TrajectoryService::dataLoadFailed, this,
            [this](const QString &message) {
                ElaMessageBar::error(ElaMessageBarType::PositionPolicy::TopRight, QString(), message, 4000, this);
            });

    connect(m_trajectoryService, &TrajectoryService::trajectoryPreviewReady, this,
            [this](const QString &trajectoryId, const QVector<QPointF> &points) {
                if (m_visualizationPage) {
                    m_visualizationPage->showTrajectory(trajectoryId, points);
                    m_visualizationPage->focusOnTrajectory(trajectoryId);
                }
            });

    connect(m_trajectoryService, &TrajectoryService::trajectoryTrackReady, this,
            [this](const QString &trajectoryId, const QVector<QPointF> &points, const QVector<QDateTime> &timeline) {
                if (m_playbackPage) {
                    m_playbackPage->addTrack(trajectoryId, points, timeline);
                }
            });

    connect(m_playbackPage, &PlaybackPage::playbackStarted, this, [this](const QString &trajectoryId) {
        if (m_visualizationPage) {
            m_visualizationPage->startPlayback(trajectoryId);
        }
    });

    connect(m_playbackPage, &PlaybackPage::playbackStopped, this, [this](const QString &) {
        if (m_visualizationPage) {
            m_visualizationPage->stopPlayback();
        }
    });

    connect(m_playbackPage, &PlaybackPage::playbackFrameChanged, this,
            [this](const QString &trajectoryId, const QPointF &point, const QDateTime &ts) {
                if (m_visualizationPage) {
                    m_visualizationPage->updatePlaybackFrame(trajectoryId, point, ts);
                }
            });

    connect(m_queryManager, &QueryManager::historyChanged, this,
            [this](const QVector<QueryRecord> &records) {
                if (m_dataManagerPage) {
                    m_dataManagerPage->setHistory(records);
                }
                refreshMetrics();
            });
}

void AppWindow::setupServices()
{
    m_queryManager = new QueryManager(this);
    m_trajectoryService = new TrajectoryService(this);
    m_statisticService = new StatisticService(this);
    m_settingsService = new SettingsService(this);
    m_incidentService = new IncidentService(this);
    m_reportService = new ReportService(this);

    if (m_reportService) {
        m_reportService->setDataSources(m_queryManager, m_trajectoryService);
    }

    if (m_settingsService && m_incidentService) {
        connect(m_settingsService, &SettingsService::incidentSettingsChanged, this, [this](const SettingsService::IncidentSettings &settings) {
            IncidentService::IncidentConnectionSettings conn;
            conn.endpoint = settings.endpoint;
            conn.useMock = settings.useMock;
            m_incidentService->applySettings(conn);
        });
        const auto initial = m_settingsService->incidentSettings();
        IncidentService::IncidentConnectionSettings conn;
        conn.endpoint = initial.endpoint;
        conn.useMock = initial.useMock;
        m_incidentService->applySettings(conn);
    }

    if (m_settingsService) {
        connect(m_settingsService, &SettingsService::settingsChanged, this, [this](const SettingsService::AppSettings &settings) {
            if (m_visualizationPage) {
                m_visualizationPage->setShowGridEnabled(settings.showMapGrid);
                m_visualizationPage->setSmoothRenderingEnabled(settings.smoothAnimation);
            }
        });
    }
}

void AppWindow::restoreLastSession()
{
    if (!m_settingsService) {
        if (!m_dashboardKey.isEmpty()) {
            navigation(m_dashboardKey);
        }
        return;
    }

    const auto settings = m_settingsService->settings();
    if (!settings.restoreSession || settings.lastPageKey.isEmpty()) {
        if (!m_dashboardKey.isEmpty()) {
            navigation(m_dashboardKey);
        }
        return;
    }

    const QString targetKey = settings.lastPageKey;
    const QList<QString> knownKeys{m_dashboardKey, m_queryKey, m_visualizationKey, m_playbackKey, m_incidentKey, m_reportKey, m_dataManagerKey, m_settingsKey};
    if (knownKeys.contains(targetKey) && !targetKey.isEmpty()) {
        navigation(targetKey);
    } else if (!m_dashboardKey.isEmpty()) {
        navigation(m_dashboardKey);
    }
    m_currentPageKey = getCurrentNavigationPageKey();
}

void AppWindow::persistCurrentPage() const
{
    if (!m_settingsService || m_currentPageKey.isEmpty()) {
        return;
    }
    m_settingsService->setLastPageKey(m_currentPageKey);
}

void AppWindow::refreshMetrics()
{
    if (!m_statisticService || !m_queryManager) {
        return;
    }
    const auto metrics = m_statisticService->currentMetrics(m_queryManager->records());
    if (m_dashboardPage) {
        m_dashboardPage->updateMetrics(metrics);
    }
}

QueryRecord AppWindow::buildRecord(const QueryParameters &parameters, int resultCount) const
{
    QueryRecord record;
    record.type = parameters.type;
    record.resultCount = resultCount;
    record.parameters.insert(QStringLiteral("vehicle"), parameters.vehicleId);
    record.parameters.insert(QStringLiteral("queryType"), parameters.queryType);
    record.parameters.insert(QStringLiteral("lonMin"), parameters.longitudeMin);
    record.parameters.insert(QStringLiteral("lonMax"), parameters.longitudeMax);
    record.parameters.insert(QStringLiteral("latMin"), parameters.latitudeMin);
    record.parameters.insert(QStringLiteral("latMax"), parameters.latitudeMax);
    record.parameters.insert(QStringLiteral("start"), parameters.startTime);
    record.parameters.insert(QStringLiteral("end"), parameters.endTime);

    if (parameters.endTime.isValid()) {
        record.timestamp = parameters.endTime;
    } else if (parameters.startTime.isValid()) {
        record.timestamp = parameters.startTime;
    } else {
        record.timestamp = QDateTime::currentDateTime();
    }
    return record;
}
