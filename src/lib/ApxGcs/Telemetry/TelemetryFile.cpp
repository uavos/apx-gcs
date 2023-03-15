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
#include "TelemetryFile.h"
#include "TelemetryFileFormat.h"

#include <App/AppDirs.h>
#include <Vehicles/Vehicle.h>

using namespace telemetry;

TelemetryFile::TelemetryFile() {}

bool TelemetryFile::create(Vehicle *vehicle)
{
    if (isOpen()) {
        qDebug() << "file break";
        close();
    }

    auto dir = AppDirs::telemetry();
    dir.mkpath(".");

    auto t = QDateTime::currentDateTime();

    QString stime = t.toString("yyyy_MM_dd_HH_mm");

    QString callsign = vehicle->title();
    if (callsign.isEmpty())
        callsign = vehicle->confTitle();

    QString fname;
    for (int i = 0; i < 1000; ++i) {
        QString s = QString("%1_%2").arg(stime).arg(i, 3, 10, QChar('0'));

        if (!callsign.isEmpty())
            s.append('_').append(callsign);

        s.append('.').append(suffix);

        if (!QFile::exists(dir.absoluteFilePath(s))) {
            fname = s;
            break;
        }
    }

    if (fname.isEmpty()) {
        qWarning() << "failed to create file name";
        return false;
    }

    setFileName(dir.absoluteFilePath(fname));

    if (!open(QIODevice::WriteOnly)) {
        qWarning() << "failed to open file" << fileName();
        return false;
    }

    // write file header
    fhdr_s fhdr{};

    strcpy(fhdr.magic.magic, "APXTLM");
    fhdr.magic.version = 1;
    fhdr.info.time = t.toMSecsSinceEpoch();

    write((const char *) &fhdr, sizeof(fhdr));

    return true;
}
