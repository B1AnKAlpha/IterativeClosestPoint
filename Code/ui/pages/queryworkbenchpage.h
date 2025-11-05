#ifndef UI_PAGES_QUERYWORKBENCHPAGE_H
#define UI_PAGES_QUERYWORKBENCHPAGE_H

#include <QWidget>

#include "common/queryparameters.h"

class QueryForm;
class QListWidget;
class ElaText;

class QueryWorkbenchPage : public QWidget
{
    Q_OBJECT

public:
    explicit QueryWorkbenchPage(QWidget *parent = nullptr);

    void appendResult(const QString &label);
    void clearResults();

signals:
    void queryTriggered(const QueryParameters &parameters);
    void requestVisualization(const QString &trajectoryId);

private:
    void buildLayout();
    void connectSignals();
    void updatePlaceholderVisibility();

    QueryForm *m_form{nullptr};
    QListWidget *m_resultList{nullptr};
    ElaText *m_emptyLabel{nullptr};
};

#endif // UI_PAGES_QUERYWORKBENCHPAGE_H
