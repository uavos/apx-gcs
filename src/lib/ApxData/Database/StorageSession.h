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

#include "DatabaseSession.h"

namespace db {
namespace storage {

class Session : public DatabaseSession
{
public:
    explicit Session(QObject *parent, QString sessionName);

    static QString telemetryFileBasename(QDateTime timestamp, QString unitName);
    static QString telemetryFilePath(const QString &basename);
    static QString telemetryFilePathUnique(const QString &basename);

    static QJsonObject telemetryFilenameParse(const QString &filePath);

private:
    Fact *f_stats;
    Fact *f_trash;
    Fact *f_sync;

    Fact *f_stop;

    void getStats();
    void emptyTrash();
    void syncFiles();
};

class Request : public DatabaseRequest
{
public:
    explicit Request();
};

} // namespace storage
} // namespace db
