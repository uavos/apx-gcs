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
#include "Poi.h"
#include "MissionField.h"
#include "VehicleMission.h"
#include <App/App.h>
#include <QGeoCircle>

Poi::Poi(MissionGroup *parent)
    : MissionItem(parent, "p#", "", "")
{
    f_hmsl = new MissionField(this, "hmsl", tr("HMSL"), tr("Object of interest altitude MSL"), Int);
    f_hmsl->setUnits("m");
    f_hmsl->setEnumStrings(QStringList() << "ground");
    f_hmsl->setOpt("extrainfo", "ExtraInfoElevation.qml");

    f_radius = new MissionField(this, "radius", tr("Radius"), tr("Loiter radius"), Int);
    f_radius->setUnits("m");
    f_radius->setMin(-10000);
    f_radius->setMax(10000);
    f_radius->setValue(800);
    f_radius->setOpt("editor", "EditorIntWithFeet.qml");

    f_loops = new MissionField(this, "loops", tr("Loops"), tr("Loiter loops limit"), Int);
    f_loops->setEnumStrings(QStringList() << "default");
    f_loops->setMin(0);
    f_loops->setMax(255);

    f_time = new MissionField(this, "timeout", tr("Time"), tr("Loiter time limit"), Int);
    f_time->setEnumStrings(QStringList() << "default");
    f_time->setUnits("min");
    f_time->setMin(0);
    f_time->setMax(0xFFFF);

    // Add feets option
    // 3.2808 - conversion coefficient feets to meters
    f_hmsl->setOpt("editor", "EditorIntWithFeet.qml");
    f_radius->setOpt("editor", "EditorIntWithFeet.qml");

    const float coef = 3.2808;
    auto ft = std::round(f_radius->value().toInt() * coef);
    f_radius->setOpt("ft", ft);
    f_hmsl->setOpt("ft", 0);

    //switch ft/m on
    f_feet->setVisible(true);

    //conversions
    connect(this, &MissionItem::coordinateChanged, this, &Poi::radiusPointChanged);
    connect(f_radius, &Fact::valueChanged, this, &Poi::radiusPointChanged);

    //title
    connect(f_radius, &Fact::valueChanged, this, &Poi::updateTitle);
    updateTitle();

    connect(f_hmsl, &Fact::valueChanged, this, &Poi::updateDescr);
    connect(f_loops, &Fact::valueChanged, this, &Poi::updateDescr);
    connect(f_time, &Fact::valueChanged, this, &Poi::updateDescr);
    updateDescr();

    App::jsync(this);
}

void Poi::updateTitle()
{
    QStringList st;
    st.append(QString::number(num() + 1));
    int r = f_radius->value().toInt();
    if (std::abs(r) > 0) {
        st.append(AppRoot::distanceToString(std::abs(r)));
        if (r < 0)
            st.append(tr("CCW"));
    } else
        st.append("H");
    setTitle(st.join(' '));
}
void Poi::updateDescr()
{
    QStringList st;
    QString sts;
    if (!f_hmsl->isZero()) {
        st.append("MSL" + f_hmsl->valueText());
        sts.append("H");
    }
    if (!f_loops->isZero()) {
        st.append("L" + f_loops->valueText());
        sts.append("L");
    }
    if (!f_time->isZero()) {
        st.append("T" + f_time->valueText());
        sts.append("T");
    }
    setDescr(st.join(' '));
    setValue(sts);
}

QGeoRectangle Poi::boundingGeoRectangle() const
{
    return MissionItem::boundingGeoRectangle().united(
        QGeoCircle(coordinate(), std::abs(f_radius->value().toDouble())).boundingGeoRectangle());
}

QGeoCoordinate Poi::radiusPoint() const
{
    QGeoCoordinate p(f_latitude->value().toDouble(), f_longitude->value().toDouble());
    return p.atDistanceAndAzimuth(std::abs(f_radius->value().toInt()), 90.0);
}
void Poi::setRadiusPoint(const QGeoCoordinate &v)
{
    QGeoCoordinate p(f_latitude->value().toDouble(), f_longitude->value().toDouble());
    double a = qDegreesToRadians(p.azimuthTo(v));
    double d = p.distanceTo(v);
    QPointF ne(d * std::cos(a), d * std::sin(a));
    ne = AppRoot::rotate(ne, 90.0);
    int rabs = std::abs(f_radius->value().toInt());
    if (std::abs(ne.y()) > (rabs / 2.0)) {
        //switch turn direction
        f_radius->setValue(ne.y() > 0 ? rabs : -rabs);
    }
    int dist = ne.x();
    if (dist < 20)
        dist = 0;
    else if (dist > 50000)
        dist = 50000;
    else if (dist > 500)
        dist = (dist / 100) * 100;
    else
        dist = (dist / 10) * 10;
    f_radius->setValue(f_radius->value().toInt() < 0 ? -dist : dist);
}
