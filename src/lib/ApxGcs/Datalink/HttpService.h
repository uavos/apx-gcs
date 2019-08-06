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
#ifndef HTTPSERVICE_H
#define HTTPSERVICE_H
#include <Vehicles/VehicleMandalaValue.h>
#include <QtCore>
//=============================================================================
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

    CurrentVehicleMandalaValue<double> c_gps_lat;
    CurrentVehicleMandalaValue<double> c_gps_lon;
    CurrentVehicleMandalaValue<double> c_gps_hmsl;
    CurrentVehicleMandalaValue<double> c_course;
    CurrentVehicleMandalaValue<double> c_roll;
    CurrentVehicleMandalaValue<double> c_pitch;

public slots:
    void httpRequest(QTextStream &stream, QString req, bool *ok);
};
//=============================================================================
#endif
