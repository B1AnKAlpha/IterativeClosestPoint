#ifndef UI_PAGES_REPORTPAGE_H
#define UI_PAGES_REPORTPAGE_H

#include <QWidget>

#include "services/reportservice.h"

class QListWidget;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class QSpinBox;
class QDateEdit;

class ReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit ReportPage(QWidget *parent = nullptr);

    void setReportService(ReportService *service);

private:
    void buildLayout();
    void connectSignals();
    void applyConfig(const ReportService::ReportConfig &config);
    QStringList parseOwners(const QString &text) const;

    QListWidget *m_templateList{nullptr};
    ElaText *m_hintLabel{nullptr};
    ElaLineEdit *m_scheduleEdit{nullptr};
    ElaLineEdit *m_ownerEdit{nullptr};
    QSpinBox *m_archiveSpin{nullptr};
    QDateEdit *m_dateEdit{nullptr};
    ElaPushButton *m_generateButton{nullptr};
    ElaPushButton *m_scheduleSaveButton{nullptr};
    ReportService *m_service{nullptr};
};

#endif // UI_PAGES_REPORTPAGE_H
