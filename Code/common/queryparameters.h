#ifndef COMMON_QUERYPARAMETERS_H
#define COMMON_QUERYPARAMETERS_H

#include <QDateTime>
#include <QString>

#include "querytypes.h"

struct QueryParameters
{
    QueryType type{QueryType::Trajectory};
    QString vehicleId;
    QString queryType;
    QString longitudeMin;
    QString longitudeMax;
    QString latitudeMin;
    QString latitudeMax;
    QDateTime startTime;
    QDateTime endTime;
};

#endif // COMMON_QUERYPARAMETERS_H
