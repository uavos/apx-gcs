/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <Database/TelemetryReqRead.h>
#include <Fact/Fact.h>
#include <QGeoPath>

class TelemetryReaderDataReq : public DBReqTelemetryReadData
{
    Q_OBJECT
public:
    explicit TelemetryReaderDataReq(quint64 tID)
        : DBReqTelemetryReadData(tID)
    {}
    //types
    typedef QHash<quint64, QVector<QPointF> *> fieldData_t;
    typedef QMap<QString, quint64> evtCountMap_t;
    typedef QMap<quint64, QString> fieldNames_t;
    typedef QVector<double> times_t;
    typedef struct
    {
        quint64 time;
        QString name;
        QString value;
        QString uid;
    } event_t;
    typedef QList<event_t> events_t;

protected:
    bool run(QSqlQuery &query);

private:
    Fact *f_events;
    void addEventFact(quint64 time, const QString &name, const QString &value, const QString &uid);

signals:
    void dataProcessed(quint64 telemetryID,
                       quint64 cacheID,
                       fieldData_t fieldData,
                       fieldNames_t fieldNames,
                       times_t times,
                       events_t events,
                       QGeoPath path,
                       Fact *f_events);
};
