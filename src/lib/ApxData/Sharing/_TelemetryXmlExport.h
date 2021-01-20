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
#ifndef TelemetryXmlExport_H
#define TelemetryXmlExport_H
//=============================================================================
#include <ApxMisc/QueueWorker.h>
#include <Database/TelemetryReqRead.h>
//=============================================================================
class TelemetryXmlExport : public QueueWorker
{
    Q_OBJECT

public:
    explicit TelemetryXmlExport();

protected:
    void exec(Fact *f);
    void run();

private:
    QString _fileName;
    quint64 _telemetryID;

    bool write(quint64 telemetryID, QString fileName);

    bool writeCSV(QFile *file_p, DBReqTelemetryReadData &req);

    bool writeXml(QFile *file_p,
                  DBReqTelemetryReadData &req,
                  const QString &title,
                  const QString &sharedHash,
                  const QVariantMap &info,
                  const QVariantMap &stats);
    void writeDownlink(QXmlStreamWriter &stream, quint64 time, const QStringList &values);
    void writeUplink(QXmlStreamWriter &stream,
                     quint64 time,
                     const QString &name,
                     const QString &value);
    void writeEvent(QXmlStreamWriter &stream,
                    quint64 time,
                    const QString &name,
                    const QString &value,
                    const QString &uid,
                    bool uplink);
};
//=============================================================================
#endif
