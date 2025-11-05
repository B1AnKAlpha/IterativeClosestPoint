#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include "services/registrationservice.h"

class ElaText;
class ElaTableView;
class QLabel;
class QStandardItemModel;

/**
 * @brief 概览页面
 * 
 * 显示点云信息和配准历史
 */
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
    
    void setRegistrationService(RegistrationService* service);
    
private slots:
    void updateCloudInfo();
    void updateHistory(const QVector<RegistrationRecord>& history);
    
private:
    void buildUI();
    QWidget* createInfoCard(const QString& title, const QString& value, const QString& icon);
    
    RegistrationService* m_registrationService;
    
    // 信息卡片
    QLabel* m_sourcePointsValue;
    QLabel* m_targetPointsValue;
    QLabel* m_registrationCountValue;
    
    // 历史记录表格
    ElaTableView* m_historyTable;
    QStandardItemModel* m_historyModel;
};

#endif // DASHBOARDPAGE_H
