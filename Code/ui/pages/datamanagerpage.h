#ifndef UI_PAGES_DATAMANAGERPAGE_H
#define UI_PAGES_DATAMANAGERPAGE_H

#include <QWidget>

#include <QVector>

#include "querymanager.h"

class ElaTableView;
class ElaPushButton;
class QStandardItemModel;

class DataManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit DataManagerPage(QWidget *parent = nullptr);

    void setHistory(const QVector<QueryRecord> &records);

signals:
    void importRequested();
    void exportRequested();
    void requestRebuildTree();

private:
    void buildLayout();
    void connectSignals();
    QString formatParameters(const QVariantMap &parameters) const;

    ElaTableView *m_table{nullptr};
    ElaPushButton *m_importButton{nullptr};
    ElaPushButton *m_exportButton{nullptr};
    ElaPushButton *m_rebuildButton{nullptr};
    QStandardItemModel *m_model{nullptr};
};

#endif // UI_PAGES_DATAMANAGERPAGE_H
