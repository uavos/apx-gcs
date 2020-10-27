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
#include "Waypoint.h"
#include "MissionField.h"
#include "VehicleMission.h"
#include <App/App.h>

Waypoint::Waypoint(MissionGroup *parent)
    : MissionItem(parent, "w#", "", "")
    , icourse(0)
    , m_reachable(false)
    , m_warning(false)
{
    f_altitude = new MissionField(this, "altitude", tr("Altitude"), tr("Altitude above ground"), Int);
    f_altitude->setUnits("m");

    f_type = new MissionField(this, "type", tr("Type"), tr("Maneuver type"), Enum);
    f_type->setEnumStrings(QMetaEnum::fromType<ProtocolMission::ManeuverType>());

    //actions
    f_actions = new WaypointActions(this);

    //default values
    Waypoint *f0 = static_cast<Waypoint *>(prevItem());
    if (f0)
        f_altitude->setValue(f0->f_altitude->value());
    else
        f_altitude->setValue(200);

    connect(f_type, &Fact::valueChanged, this, &Waypoint::updatePath);

    connect(f_type, &Fact::valueChanged, this, &Waypoint::updateTitle);
    connect(f_altitude, &Fact::valueChanged, this, &Waypoint::updateTitle);
    updateTitle();

    connect(f_actions, &Fact::valueChanged, this, &Waypoint::updateDescr);
    updateDescr();

    App::jsync(this);
}

void Waypoint::updateTitle()
{
    QStringList st;
    st.append(QString::number(num() + 1));
    st.append(f_type->text().left(1).toUpper());
    st.append(f_altitude->text() + f_altitude->units());
    setTitle(st.join(' '));
}
void Waypoint::updateDescr()
{
    setDescr(f_actions->value().toString());
}

QGeoPath Waypoint::getPath()
{
    QGeoPath p;

    double spd = 0; //QMandala::instance()->current->apcfg.value("spd_cruise").toDouble();
    /*if(f_speed->value().toUInt()>0)
    spd=f_speed->value().toUInt();*/
    if (spd <= 0)
        spd = 22;
    double dt = 1.0;
    double turnR = 0; //QMandala::instance()->current->apcfg.value("turnR").toDouble();
    if (turnR <= 0)
        turnR = 100;
    double turnRate = (360.0 / (2.0 * M_PI)) * spd / turnR;
    double crs = m_course;
    double distance = 0;
    MissionItem *prev = prevItem();
    QGeoCoordinate dest(coordinate());
    QGeoCoordinate pt;
    bool wptReached = true;
    bool wptWarning = false;
    bool wptLine = false;
    while (1) {
        if (!prev) {
            pt = group->mission->startPoint();
            if (!pt.isValid()) {
                crs = 0;
                //pt=QGeoCoordinate(vm->factById(idx_home_pos|(0<<8))->value().toDouble(),vm->factById(idx_home_pos|(1<<8))->value().toDouble());
                //p.addCoordinate(dest);
                p.addCoordinate(dest);
                //crs=dest.azimuthTo(next->coordinate());
                break;
            }
            p.addCoordinate(pt);
            crs = group->mission->startHeading();
            double slen = group->mission->startLength();
            if (slen > 0) {
                pt = pt.atDistanceAndAzimuth(slen, crs);
                p.addCoordinate(pt);
                distance += slen;
            }
        } else {
            pt = prev->coordinate();
            if (prev->geoPath().path().size() > 1) {
                crs = prev->course();
                wptLine = f_type->value().toInt() == ProtocolMission::Path;
            } else
                wptLine = true;
        }
        //fly to wpt
        p.addCoordinate(pt);
        //int cnt=0;
        double turnCnt = 0;
        while (1) {
            double deltaHdg = AppRoot::angle(pt.azimuthTo(dest) - crs);
            double deltaDist = pt.distanceTo(dest);
            double step = dt * spd;
            if (wptLine || std::abs(deltaHdg) < (dt * 10.0)) {
                //crs ok (turn finished)
                step = 10.0e+3 * dt;
                crs += deltaHdg;
                deltaHdg = 0;
                if (deltaDist <= step) {
                    //wpt reached
                    crs += deltaHdg;
                    deltaHdg = 0;
                    distance += deltaDist;
                    pt = dest;
                    p.addCoordinate(dest);
                    wptReached = true;
                    break;
                }
            }
            //propagate position
            pt = pt.atDistanceAndAzimuth(step, crs);
            distance += step;
            deltaHdg = dt * AppRoot::limit(deltaHdg, -turnRate, turnRate);
            crs += deltaHdg;
            p.addCoordinate(pt);
            turnCnt += deltaHdg;
            if (std::abs(turnCnt) > (360 * 2)) { //(++cnt)>(360/turnRate)){
                wptReached = false;
                break;
            }
        }
        if (p.path().size() < 2)
            p.addCoordinate(dest);
        //qDebug()<<plist;
        break;
    }

    //update properties
    wptWarning |= distance < turnR * (2.0 * M_PI * 0.8);

    setReachable(wptReached);
    setWarning(wptWarning);

    setDistance(distance);
    setTime(distance / spd);

    //end course
    if (p.path().size() == 2 && crs == m_course) {
        crs = p.path().first().azimuthTo(p.path().at(1));
    }
    crs = AppRoot::angle(crs);
    int icrs = (int) (crs / 10) * 10;
    if (icourse != icrs) {
        icourse = icrs;
        //force next item to be updated
        MissionItem *next = nextItem();
        if (next)
            next->resetPath();
    }
    setCourse(crs);

    return p;
}

bool Waypoint::reachable() const
{
    return m_reachable;
}
void Waypoint::setReachable(bool v)
{
    if (m_reachable == v)
        return;
    m_reachable = v;
    emit reachableChanged();
}
bool Waypoint::warning() const
{
    return m_warning;
}
void Waypoint::setWarning(bool v)
{
    if (m_warning == v)
        return;
    m_warning = v;
    emit warningChanged();
}
