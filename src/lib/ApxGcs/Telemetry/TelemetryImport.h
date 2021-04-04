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

#include <ApxMisc/QueueWorker.h>
#include <Database/DatabaseRequest.h>

class TelemetryImport : public QueueWorker
{
    Q_OBJECT

public:
    explicit TelemetryImport();

protected:
    void exec(Fact *f);
    void run();

private:
    QString _fileName;
    QString title;
    QString sharedHash;
    QString notes;
    quint64 telemetryID;

    QVariantMap recordInfo;
    QVariantMap userInfo;
    QVariantMap nodesVehicleInfo;

    quint64 read(QString fileName);
    quint64 read(QXmlStreamReader &xml);

    QVariantMap readSection(QXmlStreamReader &xml);
    QByteArray readXmlPart(QXmlStreamReader &xml);

    void readConfigs(QXmlStreamReader &xml);
    void readMissions(QXmlStreamReader &xml);

    //database
    quint64 dbReadSharedHashId(QString hash);
    quint64 dbSaveID(
        QString vehicleUID, QString callsign, QString comment, bool rec, quint64 timestamp);
    void dbSaveData(quint64 time_ms, quint64 fieldID, double value, bool uplink);
    void dbSaveEvent(quint64 time_ms,
                     const QString &name,
                     const QString &value,
                     const QString &uid,
                     bool uplink);

    bool dbCommitRecord();
};
