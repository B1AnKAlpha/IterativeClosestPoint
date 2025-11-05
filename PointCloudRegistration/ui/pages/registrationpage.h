#ifndef REGISTRATIONPAGE_H
#define REGISTRATIONPAGE_H

#include <QWidget>
#include "services/registrationservice.h"
#include "services/settingsservice.h"

class ElaPushButton;
class ElaProgressBar;
class ElaText;
class QTextEdit;
class QLabel;
class QTableWidget;

/**
 * @brief 配准控制台页面
 * 
 * 控制配准流程并显示实时进度
 */
class RegistrationPage : public QWidget
{
    Q_OBJECT

public:
    explicit RegistrationPage(QWidget *parent = nullptr);
    
    void setRegistrationService(RegistrationService* service);
    void setSettingsService(SettingsService* service);
    
private slots:
    void onStartRegistration();
    void onStopRegistration();
    void onRegistrationStarted();
    void onRegistrationProgress(int iteration, int total, double rmse);
    void onIterationCompleted(const IterationResult& result);
    void onRegistrationFinished(bool success, const QString& message);
    void onLogMessage(const QString& message);
    
private:
    void buildUI();
    void updateControls(bool isRegistering);
    void appendLog(const QString& message);
    
    RegistrationService* m_registrationService;
    SettingsService* m_settingsService;
    
    // 控制按钮
    ElaPushButton* m_startButton;
    ElaPushButton* m_stopButton;
    
    // 进度显示
    ElaProgressBar* m_progressBar;
    QLabel* m_iterationLabel;
    QLabel* m_rmseLabel;
    QLabel* m_validPointsLabel;
    QLabel* m_outlierPointsLabel;
    
    // 日志输出
    QTextEdit* m_logTextEdit;
    
    // 结果表格
    QTableWidget* m_resultTable;
};

#endif // REGISTRATIONPAGE_H
