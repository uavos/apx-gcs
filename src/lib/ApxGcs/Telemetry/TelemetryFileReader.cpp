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
#include "TelemetryFileReader.h"
#include "TelemetryFileFormat.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <Vehicles/Vehicle.h>

#include <TelemetryValuePack.h>
#include <TelemetryValueUnpack.h>

using namespace telemetry;

bool TelemetryFileReader::open(QString name)
{
    _info = {};
    _tags.clear();
    _fields_map.clear();
    _values_s.clear();
    _meta_objects.clear();

    if (isOpen()) {
        qDebug() << "file break";
        close();
    }

    if (name.isEmpty()) {
        qWarning() << "failed to open file name";
        return false;
    }

    name.append('.').append(APXTLM_FTYPE);

    QFile::setFileName(AppDirs::telemetry().absoluteFilePath(name));

    // open file for reading
    if (!QFile::open(QIODevice::ReadOnly)) {
        qWarning() << "failed to open file" << fileName();
        return false;
    }
    do {
        // read file header
        fhdr_s fhdr;
        if (read((char *) &fhdr, sizeof(fhdr)) != sizeof(fhdr)) {
            qWarning() << "failed to read file header";
            break;
        }

        if (strcmp(fhdr.magic.magic, APXTLM_MAGIC)) {
            qWarning() << "invalid file magic";
            break;
        }

        if (fhdr.magic.version > APXTLM_VERSION) {
            qWarning() << "invalid file version" << fhdr.magic.version << APXTLM_VERSION;
            break;
        }

        if (fhdr.magic.hsize > sizeof(fhdr)) {
            qWarning() << "invalid file header size" << fhdr.magic.hsize << sizeof(fhdr);
            break;
        }

        // file format seem to be valid
        _info = fhdr.info;

        // seek back to the beginning of the payload if necessary
        if (fhdr.magic.hsize < sizeof(fhdr)) {
            QFile::seek(fhdr.magic.hsize);
        }

        // read tags
        XbusStreamReader stream(fhdr.tags, sizeof(fhdr.tags));
        while (stream.available() > 0) {
            auto c = stream.read_string(stream.available());
            if (!c)
                break;
            QString s = QString::fromUtf8((const char *) c);

            QString key, value;
            if (s.contains(':')) {
                key = s.section(':', 0, 0);
                value = s.section(':', 1);
            } else {
                key = s;
            }
            _tags[key] = value;
        }

        return true;
    } while (0);

    // some error occured
    close();
    return false;
}
