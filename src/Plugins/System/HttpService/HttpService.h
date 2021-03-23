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

#include <Vehicles/Vehicle.h>
#include <QtCore>

class HttpService : public QObject
{
    Q_OBJECT
public:
    HttpService(QObject *parent = nullptr);

private:
    //mandala
    QString reply_mandala(const QString &req);

    //googleearth
    QString reply_google(const QString &req);
    QString reply_kml();
    QString reply_telemetry();
    QString reply_flightplan();
    QString reply_chase();
    QString reply_chase_upd();

    Fact *c_gps_lat;
    Fact *c_gps_lon;
    Fact *c_gps_hmsl;
    Fact *c_course;
    Fact *c_roll;
    Fact *c_pitch;
private slots:
    void vehicleSelected(Vehicle *vehicle);

public slots:
    void httpRequest(QTextStream &stream, QString req, bool *ok);
};
