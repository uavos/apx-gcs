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
#include "TelemetryFileImport.h"
#include "TelemetryFileReader.h"

#include <App/App.h>
#include <App/AppDirs.h>

TelemetryFileImport::TelemetryFileImport(QObject *parent)
    : QTemporaryFile(parent)
{}

bool TelemetryFileImport::import(QString srcFileName)
{
    auto fi = QFileInfo(srcFileName);
    if (!fi.exists() || fi.size() == 0) {
        apxMsgW() << tr("Missing data source").append(':') << fi.absoluteFilePath();
        return false;
    }

    emit progress(0);
    bool rv = false;

    do {
        // get src hash
        {
            QFile src_file(srcFileName);
            if (!src_file.open(QIODevice::ReadOnly)) {
                qWarning() << "failed to open file" << fi.fileName();
                break;
            }
            QCryptographicHash sha1(QCryptographicHash::Sha1);
            sha1.addData(&src_file);
            src_file.close();
            _src_hash = QString(sha1.result().toHex().toUpper());
        }

        // import native format
        if (fi.suffix() == telemetry::APXTLM_FTYPE) {
            TelemetryFileReader reader(fi.absoluteFilePath());
            if (!reader.open()) {
                apxMsgW() << tr("Failed to open").append(':') << fi.absoluteFilePath();
                break;
            }
            reader.parse_payload();
            _info = reader.info();

            // check hash exists in info
            auto hash = _info.value("hash").toString();
            if (hash != _src_hash) {
                qDebug() << "hash mismatch" << hash << _src_hash;
                apxMsgW() << tr("Failed to get file hash");
                break;
            }

            //copy data from source
            if (!open()) {
                apxMsgW() << tr("Failed to open temporary file");
                break;
            }

            reader.seek(0);
            write(reader.readAll());
            flush();
            close();

            rv = true;
            break;
        }

        // try to import older formats

        apxMsgW() << tr("Unsupported format").append(':') << fi.fileName();
        break;
    } while (0);

    // finished
    emit progress(-1);
    return rv;
}
