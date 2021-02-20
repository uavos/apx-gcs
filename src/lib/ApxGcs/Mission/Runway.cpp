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
#include "Runway.h"
//#include "Mission.h"
#include "MissionField.h"
#include "VehicleMission.h"
#include <App/App.h>

Runway::Runway(MissionGroup *parent)
    : MissionItem(parent, "r#", "", "")
{
    f_type = new MissionField(this, "type", tr("Type"), tr("Landing pattern type"), Enum);
    f_type->setEnumStrings(QMetaEnum::fromType<ProtocolMission::RunwayType>());

    f_approach = new MissionField(this,
                                  "approach",
                                  tr("Approach length"),
                                  tr("Final approach length"),
                                  Int);
    f_approach->setUnits("m");
    f_approach->setMin(0);
    f_approach->setMax(10000);

    f_hmsl = new MissionField(this,
                              "hmsl",
                              tr("HMSL"),
                              tr("Runway altitude above mean sea level"),
                              Int);
    f_hmsl->setUnits("m");
    f_hmsl->setEnumStrings(QStringList() << "default");

    f_dN = new MissionField(this, "dN", tr("Delta North"), tr("Runway direction point (north)"), Int);
    f_dN->setUnits("m");
    f_dN->setMin(-10000);
    f_dN->setMax(10000);
    f_dE = new MissionField(this, "dE", tr("Delta East"), tr("Runway direction point (east)"), Int);
    f_dE->setUnits("m");
    f_dE->setMin(-10000);
    f_dE->setMax(10000);

    //default values
    f_approach->setValue(800);
    f_dN->setValue(100);
    f_dE->setValue(300);

    //title
    connect(f_type, &Fact::valueChanged, this, &Runway::updateTitle);
    connect(f_approach, &Fact::valueChanged, this, &Runway::updateTitle);
    connect(f_hmsl, &Fact::valueChanged, this, &Runway::updateTitle);
    connect(this, &Runway::headingChanged, this, &Runway::updateTitle);
    updateTitle();

    //conversions
    connect(f_dN, &Fact::valueChanged, this, &Runway::endPointChanged);
    connect(f_dE, &Fact::valueChanged, this, &Runway::endPointChanged);
    connect(this, &MissionItem::coordinateChanged, this, &Runway::endPointChanged);

    connect(f_dN, &Fact::valueChanged, this, &Runway::headingChanged);
    connect(f_dE, &Fact::valueChanged, this, &Runway::headingChanged);

    connect(this, &MissionItem::coordinateChanged, this, &Runway::appPointChanged);
    connect(f_approach, &Fact::valueChanged, this, &Runway::appPointChanged);
    connect(this, &Runway::headingChanged, this, &Runway::appPointChanged);

    connect(this, &Runway::endPointChanged, this, &Runway::updateMissionStartPoint);
    connect(this, &Runway::appPointChanged, this, &Runway::updateMissionStartPoint);
    connect(this, &Fact::numChanged, this, &Runway::updateMissionStartPoint);

    updateMissionStartPoint();
    connect(this, &Fact::removed, group, [=]() {
        //qDebug()<<"rm";
        if (group->size() <= 0) {
            //qDebug()<<"rst";
            group->mission->setStartPoint(QGeoCoordinate());
            group->mission->setStartLength(0);
            group->mission->setStartHeading(0);
            group->mission->setCoordinate(QGeoCoordinate());
        }
    });

    App::jsync(this);
}

void Runway::updateTitle()
{
    setValue(
        f_type->valueText().left(1).toUpper()
        + QString("%1").arg((int) AppRoot::angle360(round(AppRoot::angle360(heading()) / 10.0) * 10)
                                / 10,
                            2,
                            10,
                            QLatin1Char('0')));
    QStringList st;
    st.append(QString::number(num() + 1));
    st.append(value().toString());
    st.append(AppRoot::distanceToString(f_approach->value().toInt()));
    if (!f_hmsl->isZero())
        st.append("MSL" + f_hmsl->valueText());
    setTitle(st.join(' '));
}

void Runway::updateMissionStartPoint()
{
    if (num() != 0)
        return;
    QGeoCoordinate p(coordinate());
    QGeoCoordinate p2 = endPoint();
    group->mission->setStartPoint(p2);
    group->mission->setStartHeading(heading());
    double slen = f_approach->value().toDouble() * 2.0 - p.distanceTo(endPoint());
    group->mission->setStartLength(slen < 0 ? 0 : slen);
    group->mission->setCoordinate(p);
}

QGeoRectangle Runway::boundingGeoRectangle() const
{
    QList<QGeoCoordinate> clist;
    QGeoCoordinate p(coordinate());
    QGeoCoordinate pApp(
        p.atDistanceAndAzimuth(f_approach->value().toDouble() * 1.5, heading() + 180.0));
    clist.append(pApp);
    clist.append(
        pApp.atDistanceAndAzimuth(f_approach->value().toDouble(), heading() + 180.0 + 90.0));
    clist.append(
        pApp.atDistanceAndAzimuth(f_approach->value().toDouble(), heading() + 180.0 - 90.0));
    clist.append(p.atDistanceAndAzimuth(f_approach->value().toDouble() * 1.5, heading()));
    clist.append(endPoint());
    return MissionItem::boundingGeoRectangle().united(QGeoRectangle(clist));
}

QGeoCoordinate Runway::endPoint() const
{
    QGeoCoordinate p(coordinate());
    double dN = f_dN->value().toDouble();
    double dE = f_dE->value().toDouble();
    double azimuth = qRadiansToDegrees(std::atan2(dE, dN));
    double distance = std::sqrt(std::pow(dN, 2) + std::pow(dE, 2));
    return p.atDistanceAndAzimuth(distance, azimuth);
}
void Runway::setEndPoint(const QGeoCoordinate &v)
{
    QGeoCoordinate p(coordinate());
    double a = qDegreesToRadians(p.azimuthTo(v));
    double d = p.distanceTo(v);
    f_dN->setValue(d * std::cos(a));
    f_dE->setValue(d * std::sin(a));
}
QGeoCoordinate Runway::appPoint() const
{
    QGeoCoordinate p(coordinate());
    double dN = f_dN->value().toDouble();
    double dE = f_dE->value().toDouble();
    double azimuth = qRadiansToDegrees(std::atan2(dE, dN)) + 180.0;
    double distance = f_approach->value().toDouble();
    return p.atDistanceAndAzimuth(distance, azimuth);
}
void Runway::setAppPoint(const QGeoCoordinate &v)
{
    QGeoCoordinate p(coordinate());
    double a = qDegreesToRadians(p.azimuthTo(v));
    double d = p.distanceTo(v);
    QPointF ne(d * std::cos(a), d * std::sin(a));
    ne = AppRoot::rotate(ne, heading() + 180.0);
    if (std::abs(ne.y()) > (f_approach->value().toDouble() / 2.0)) {
        //switch turn direction
        f_type->setValue(ne.y() > 0 ? ProtocolMission::Left : ProtocolMission::Right);
    }
    int dist = ne.x();
    if (dist < 5)
        dist = 0;
    else if (dist > 50000)
        dist = 50000;
    else if (dist > 500)
        dist = (dist / 100) * 100;
    else if (dist > 100)
        dist = (dist / 10) * 10;
    f_approach->setValue(dist);
}
double Runway::heading() const
{
    double dN = f_dN->value().toDouble();
    double dE = f_dE->value().toDouble();
    return qRadiansToDegrees(std::atan2(dE, dN));
}
