/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef TelemetryXmlImport_H
#define TelemetryXmlImport_H
//=============================================================================
#include <ApxMisc/QueueWorker.h>
#include <Database/DatabaseRequest.h>
//=============================================================================
class TelemetryXmlImport : public QueueWorker
{
    Q_OBJECT

public:
    explicit TelemetryXmlImport();

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
    quint64 readOldFormat(QXmlStreamReader &xml);

    QVariantMap readSection(QXmlStreamReader &xml);

    QByteArray readXmlPart(QXmlStreamReader &xml);

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
    void dbSaveMission(quint64 time_ms,
                       quint64 timestamp,
                       const QString &title,
                       const QByteArray &data,
                       bool uplink);
    void dbSaveNodes(quint64 time_ms,
                     quint64 timestamp,
                     const QString &title,
                     const QByteArray &data);

    bool dbCommitRecord();
};
//=============================================================================
#endif
