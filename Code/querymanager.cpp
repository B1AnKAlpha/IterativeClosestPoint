#include "querymanager.h"

QueryManager::QueryManager(QObject *parent) : QObject(parent) {}

void QueryManager::addRecord(const QueryRecord &record) {
    QueryRecord stored = record;
    if (!stored.timestamp.isValid()) {
        stored.timestamp = QDateTime::currentDateTime();
    }
    m_records.append(stored);
    emit historyChanged(m_records);
}

void QueryManager::clear() {
    if (m_records.isEmpty()) {
        return;
    }
    m_records.clear();
    emit historyChanged(m_records);
}

int QueryManager::recordCount() const {
    return m_records.size();
}

QueryRecord QueryManager::record(int index) const {
    if (index < 0 || index >= m_records.size()) {
        return QueryRecord{};
    }
    return m_records.at(index);
}

const QVector<QueryRecord> &QueryManager::records() const
{
    return m_records;
}

QString queryTypeToString(QueryType type)
{
    switch (type) {
    case QueryType::Point:
        return QObject::tr("点查询");
    case QueryType::Area:
        return QObject::tr("区域查询");
    case QueryType::Adjacency:
        return QObject::tr("邻接分析");
    case QueryType::Trajectory:
        return QObject::tr("轨迹查询");
    }
    return QObject::tr("未知类型");
}
