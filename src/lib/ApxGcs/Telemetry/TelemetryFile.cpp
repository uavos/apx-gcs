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

    QString stime = t.toString("yyyyMMdd_HHmm");

    QString callsign = vehicle->title();
    if (callsign.isEmpty())
        callsign = vehicle->confTitle();

    QString fname;
    for (int i = 0; i < 100; ++i) {
        QString s = QString("%1%2").arg(stime).arg(i, 2, 10, QChar('0'));

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

    // open file for writing
    if (!open(QIODevice::WriteOnly)) {
        qWarning() << "failed to open file" << fileName();
        return false;
    }

    // write file header
    fhdr_s fhdr{};

    strcpy(fhdr.magic.magic, "APXTLM");
    fhdr.magic.version = version;
    fhdr.info.time = t.toMSecsSinceEpoch();

    // write tags
    XbusStreamWriter s(fhdr.tags, sizeof(fhdr.tags));
    write_tag(&s, "call", vehicle->title().toUtf8());
    write_tag(&s, "vuid", vehicle->uid().toUtf8());
    write_tag(&s, "conf", vehicle->confTitle().toUtf8());
    write_tag(&s, "class", vehicle->vehicleTypeText().toUtf8());

    // write header to file
    write((const char *) &fhdr, sizeof(fhdr));
    flush();

    return true;
}

bool TelemetryFile::write_tag(XbusStreamWriter *stream, const char *name, const char *value)
{
    if (!value || !value[0]) // skip empty values
        return true;

    auto spos = stream->pos();

    do {
        if (!stream->write_string(name))
            break;
        stream->reset(stream->pos() - 1);

        if (!stream->write_string(":"))
            break;
        stream->reset(stream->pos() - 1);

        if (!stream->write_string(value))
            break;

        return true;
    } while (0);

    stream->reset(spos);
    return false;
}
