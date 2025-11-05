#ifndef VISUALIZATIONPAGE_H
#define VISUALIZATIONPAGE_H

#include <QWidget>
#include <QTimer>
#include "widgets/pointcloudviewer.h"
#include "services/registrationservice.h"

class ElaPushButton;
class ElaSlider;
class ElaText;
class QLabel;

/**
 * @brief 可视化页面
 * 
 * 3D点云显示和迭代回放
 */
class VisualizationPage : public QWidget
{
    Q_OBJECT

public:
    explicit VisualizationPage(QWidget *parent = nullptr);
    
    void setRegistrationService(RegistrationService* service);
    PointCloudViewer* getViewer() { return m_viewer; }
    
    void updateViewer();
    void loadIterationHistory(const std::vector<IterationResult>& history);
    
private slots:
    void onFirstFrame();
    void onLastFrame();
    void onSliderChanged(int value);
    void onIterationChanged(int iteration);
    
private:
    void buildUI();
    void updatePlaybackControls();
    void updateIterationInfo();
    
    RegistrationService* m_registrationService;
    PointCloudViewer* m_viewer;
    
    // 播放控制
    ElaPushButton* m_firstButton;
    ElaPushButton* m_lastButton;
    ElaSlider* m_iterationSlider;
    
    // 信息显示
    QLabel* m_iterationLabel;
    QLabel* m_rmseLabel;
    QLabel* m_transformLabel;
};

#endif // VISUALIZATIONPAGE_H
