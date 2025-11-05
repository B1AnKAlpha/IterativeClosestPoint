#include "ui/pages/datamanagerpage.h"

#include <ElaPushButton.h>
#include <ElaTableView.h>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QVBoxLayout>

DataManagerPage::DataManagerPage(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("数据管理"));
    buildLayout();
    connectSignals();
}

void DataManagerPage::buildLayout()
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 24, 24, 24);
    pageLayout->setSpacing(16);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(12);

    m_importButton = new ElaPushButton(tr("导入"), this);
    m_exportButton = new ElaPushButton(tr("导出"), this);
    m_rebuildButton = new ElaPushButton(tr("重建四叉树"), this);

    m_table = new ElaTableView(this);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({tr("类型"), tr("参数"), tr("结果数"), tr("时间戳")});
    m_table->setModel(m_model);
    m_table->horizontalHeader()->setStretchLastSection(true);

    buttonRow->addWidget(m_importButton);
    buttonRow->addWidget(m_exportButton);
    buttonRow->addWidget(m_rebuildButton);
    buttonRow->addStretch(1);

    pageLayout->addLayout(buttonRow);
    pageLayout->addWidget(m_table, 1);
}

void DataManagerPage::connectSignals()
{
    connect(m_importButton, &ElaPushButton::clicked, this, &DataManagerPage::importRequested);
    connect(m_exportButton, &ElaPushButton::clicked, this, &DataManagerPage::exportRequested);
    connect(m_rebuildButton, &ElaPushButton::clicked, this, &DataManagerPage::requestRebuildTree);
}

void DataManagerPage::setHistory(const QVector<QueryRecord> &records)
{
    if (!m_model) {
        return;
    }

    m_model->setRowCount(records.size());
    for (int row = 0; row < records.size(); ++row) {
        const auto &record = records.at(row);
        auto *typeItem = new QStandardItem(queryTypeToString(record.type));
        auto *paramItem = new QStandardItem(formatParameters(record.parameters));
        auto *countItem = new QStandardItem(QString::number(record.resultCount));
        auto *timeItem = new QStandardItem(record.timestamp.toString("yyyy-MM-dd HH:mm:ss"));

        m_model->setItem(row, 0, typeItem);
        m_model->setItem(row, 1, paramItem);
        m_model->setItem(row, 2, countItem);
        m_model->setItem(row, 3, timeItem);
    }
}

QString DataManagerPage::formatParameters(const QVariantMap &parameters) const
{
    QStringList parts;
    for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
        parts << QStringLiteral("%1=%2").arg(it.key(), it.value().toString());
    }
    return parts.join(QLatin1String(", "));
}
