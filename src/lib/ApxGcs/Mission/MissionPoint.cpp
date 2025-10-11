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
#include "MissionPoint.h"
#include "UnitMission.h"
#include <App/App.h>

MissionPoint::MissionPoint(Fact *parent, QString name, QString title, QString descr)
    : Fact(parent, name, title, descr, Fact::Text | Fact::ModifiedTrack)
{}
MissionPoint::MissionPoint(Fact *parent, QString title, QGeoCoordinate c)
    : MissionPoint(parent, "p#", title)
{
    setCoordinate(c);
}

bool MissionPoint::setValue(const QVariant &v)
{
    // value could be:
    //   string "lat,lon"
    //   map { "lat": ..., "lon": ... }
    //   array [double, double]

    do {
        if (v.typeId() == QMetaType::QString) {
            auto s = v.toString().trimmed();
            if (s == text())
                return false;

            auto parts = s.split(s.contains('|') ? '|' : s.contains(';') ? ';' : ',');
            if (parts.size() != 2)
                break;
            double lat = AppRoot::latFromString(parts[0].trimmed());
            if (lat == 0 || qIsNaN(lat))
                break;
            double lon = AppRoot::lonFromString(parts[1].trimmed());
            if (lon == 0 || qIsNaN(lon))
                break;

            setCoordinate(QGeoCoordinate(lat, lon));
            return true;
        }

        if (v.typeId() == QMetaType::QVariantMap) {
            auto m = v.toMap();
            if (!m.contains("lat") || !m.contains("lon"))
                break;
            double lat = AppRoot::latFromString(m["lat"].toString());
            if (lat == 0 || qIsNaN(lat))
                break;
            double lon = AppRoot::lonFromString(m["lon"].toString());
            if (lon == 0 || qIsNaN(lon))
                break;
            setCoordinate(QGeoCoordinate(lat, lon));
            return true;
        }

        if (v.typeId() == QMetaType::QVariantList) {
            auto a = v.toList();
            if (a.size() != 2)
                break;
            double lat = a[0].toDouble();
            if (lat == 0 || qIsNaN(lat))
                break;
            double lon = a[1].toDouble();
            if (lon == 0 || qIsNaN(lon))
                break;
            setCoordinate(QGeoCoordinate(lat, lon));
            return true;
        }

    } while (0);

    // some value parsing error
    qWarning() << "can't parse" << v;
    return false;
}

void MissionPoint::setCoordinate(const QGeoCoordinate &c)
{
    _coordinate = c;
    Fact::setValue(QString("%1 | %2").arg(AppRoot::latToString(c.latitude()),
                                          AppRoot::lonToString(c.longitude())));
}
