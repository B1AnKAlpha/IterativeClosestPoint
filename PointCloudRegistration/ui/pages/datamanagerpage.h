#ifndef DATAMANAGERPAGE_H
#define DATAMANAGERPAGE_H

#include <QWidget>
#include "services/registrationservice.h"

class ElaPushButton;
class ElaText;
class QLabel;
class QGroupBox;

/**
 * @brief 数据管理页面
 * 
 * 负责点云文件的导入导出
 */
class DataManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit DataManagerPage(QWidget *parent = nullptr);
    
    void setRegistrationService(RegistrationService* service);
    
signals:
    void fileLoaded();
    
private slots:
    void onLoadSource();
    void onLoadTarget();
    void onSaveResult();
    void onClearSource();
    void onClearTarget();
    void onSourceCloudLoaded(const QString& filename, int pointCount);
    void onTargetCloudLoaded(const QString& filename, int pointCount);
    void onCloudLoadError(const QString& message);
    
private:
    void buildUI();
    void updateFileInfo();
    
    RegistrationService* m_registrationService;
    
    // 源点云信息
    QLabel* m_sourceFileLabel;
    QLabel* m_sourcePointsLabel;
    QLabel* m_sourceBoundsLabel;
    ElaPushButton* m_loadSourceButton;
    ElaPushButton* m_clearSourceButton;
    
    // 目标点云信息
    QLabel* m_targetFileLabel;
    QLabel* m_targetPointsLabel;
    QLabel* m_targetBoundsLabel;
    ElaPushButton* m_loadTargetButton;
    ElaPushButton* m_clearTargetButton;
    
    // 操作按钮
    ElaPushButton* m_saveResultButton;
};

#endif // DATAMANAGERPAGE_H
