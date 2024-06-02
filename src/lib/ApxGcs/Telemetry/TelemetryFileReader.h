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

#include <QtCore>

#include <Mandala/Mandala.h>
#include <Mandala/MandalaContainers.h>

#include "TelemetryFileFormat.h"

class XbusStreamReader;

class TelemetryFileReader : private QFile
{
    Q_OBJECT

public:
    bool open(QString name);

    auto &info() const { return _info; }
    auto &tags() const { return _tags; }

private:
    telemetry::fhdr_s::info_s _info;
    QHash<QString, QString> _tags;

    // helpers

    // monitor changes and updates
    std::map<mandala::uid_t, uint16_t> _fields_map;
    std::map<mandala::uid_t, QVariant> _values_s;
    std::map<QString, QJsonObject> _meta_objects;
};
