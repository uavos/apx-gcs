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
#ifndef Database_H
#define Database_H
//=============================================================================
#include <Fact/Fact.h>
#include "DatabaseSession.h"
class NodesDB;
class TelemetryDB;
class MissionsDB;
//=============================================================================
class Database : public Fact
{
    Q_OBJECT

public:
    explicit Database(Fact *parent);
    ~Database();

    static Database *instance() { return _instance; }

    void add(DatabaseSession *session);

    NodesDB *nodes;
    TelemetryDB *telemetry;
    MissionsDB *missions;

private:
    static Database *_instance;
    QList<QPointer<DatabaseSession>> sessions;

private slots:

public slots:

signals:
};
//=============================================================================
#endif
