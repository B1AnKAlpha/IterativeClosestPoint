#ifndef QUERYMANAGER_H
#define QUERYMANAGER_H

#include <QDateTime>
#include <QVariantMap>
#include <QVector>
#include <QObject>
#include <QString>

#include "common/querytypes.h"

struct QueryRecord {
    QueryType type = QueryType::Point;
    QVariantMap parameters;
    int resultCount = 0;
    QDateTime timestamp;
};

class QueryManager : public QObject {
    Q_OBJECT

public:
    explicit QueryManager(QObject *parent = nullptr);

    void addRecord(const QueryRecord &record);
    void clear();
    int recordCount() const;
    QueryRecord record(int index) const;
    const QVector<QueryRecord> &records() const;

signals:
    void historyChanged(const QVector<QueryRecord> &records);

private:
    QVector<QueryRecord> m_records;
};

QString queryTypeToString(QueryType type);

#endif // QUERYMANAGER_H
