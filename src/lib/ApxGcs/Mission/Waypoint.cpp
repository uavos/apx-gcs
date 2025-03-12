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
#include "UnitMission.h"
#include <App/App.h>
#include <ApxMisc/JsonHelpers.h>

Waypoint::Waypoint(MissionGroup *parent)
    : MissionItem(parent, "w#", "", "")
{
    f_amsl = new MissionField(this, "amsl", tr("AMSL mode"), tr("Altitude above sea level"), Bool);

    f_altitude = new MissionField(this,
                                  "altitude",
                                  tr("Altitude"),
                                  tr("Altitude above takeoff point"),
                                  Int);
    _altUnits = "m";

    f_altitude->setOpt("editor", "EditorIntWithFeet.qml");
    f_altitude->setOpt("extrainfo", "ExtraInfoAltitude.qml");

    f_agl = new MissionField(this, "agl", tr("AGL"), tr("Height above ground level"), Int);
    f_agl->setUnits("m");
    f_agl->setVisible(false);
    f_agl->setDefaultValue(0);
    f_agl->setOpt("editor", "EditorIntWithFeet.qml");
    f_agl->setOpt("extrainfo", "ExtraInfoAgl.qml");

    f_atrack = new MissionField(this,
                                "atrack",
                                tr("Altitude tracking"),
                                tr("Linear altitude control"),
                                Bool);

    f_xtrack = new MissionField(this, "xtrack", tr("Line tracking"), tr("Maintain path track"), Bool);

    //actions
    f_actions = new WaypointActions(this);

    //default values
    Waypoint *f0 = static_cast<Waypoint *>(prevItem());
    if (f0)
        f_altitude->setValue(f0->f_altitude->value());
    else
        f_altitude->setValue(200);

    // Add feets options
    auto ft = std::round(f_altitude->value().toInt() * M2FT_COEF);
    f_altitude->setOpt("ft", ft);

    ft = std::round(f_agl->value().toInt() * M2FT_COEF);
    f_agl->setOpt("ft", ft);

    connect(f_altitude, &Fact::optsChanged, this, &Waypoint::updateTitle);
    connect(this, &MissionItem::isFeetsChanged, this, &Waypoint::updateTitle);
    connect(f_agl, &Fact::optsChanged, this, [this]() {if (this->chosen() == AGL) calcAltitudeFt();});
    // Add feets options end

    // elevation map and agl
    connect(f_altitude, &Fact::valueChanged, this, [this]() { if (this->chosen() == ALT) calcAgl();});
    connect(f_agl, &Fact::valueChanged, this, &Waypoint::calcAltitude);
    connect(f_altitude, &Fact::triggered, this, [this]() { this->setChosen(ALT); });
    connect(f_agl, &Fact::triggered, this, [this]() { this->setChosen(AGL); });
    initElevationMap();

    connect(this, &MissionItem::itemDataLoaded, this, &Waypoint::updateAMSL);
    connect(this, &MissionItem::itemDataLoaded, this, &Waypoint::updateTitle);
    connect(this, &MissionItem::itemDataLoaded, this, &Waypoint::updateDescr);

    connect(f_amsl, &Fact::valueChanged, this, &Waypoint::updateAMSL);
    connect(f_amsl, &Fact::valueChanged, this, &Waypoint::updateTitle);
    connect(f_amsl, &Fact::valueChanged, this, &Waypoint::updateAltDescr);
    updateAMSL();

    connect(f_xtrack, &Fact::valueChanged, this, &Waypoint::updatePath);

    connect(f_xtrack, &Fact::valueChanged, this, &Waypoint::updateTitle);
    connect(f_atrack, &Fact::valueChanged, this, &Waypoint::updateTitle);

    connect(f_altitude, &Fact::valueChanged, this, &Waypoint::updateTitle);
    updateTitle();

    connect(f_actions, &Fact::valueChanged, this, &Waypoint::updateDescr);
    updateDescr();

    App::jsync(this);
}

void Waypoint::initElevationMap()
{
    f_elevationmap = AppSettings::instance()->findChild("application.plugins.elevationmap");
    if(!f_elevationmap)
        return;
    f_refHmsl = unit()->f_mandala->fact(mandala::est::nav::ref::hmsl::uid);
    connect(f_refHmsl, &Fact::valueChanged, this, &Waypoint::calcAgl);
    connect(this, &MissionItem::elevationChanged, this, &Waypoint::calcAgl);
    connect(f_amsl, &Fact::valueChanged, this, &Waypoint::recalcAltitude);
    connect(this, &MissionItem::elevationChanged, this, [this]() {
        f_agl->setEnabled(!std::isnan(m_elevation));
    });

    calcAgl();
}

QJsonValue Waypoint::toJson()
{
    auto jso = MissionItem::toJson().toObject();

    // move all actions to object
    auto jso_actions = jso.take("actions").toObject();
    for (auto it = jso_actions.begin(); it != jso_actions.end(); ++it) {
        jso.insert(it.key(), it.value());
    }
    return json::remove_empty(jso, true);
}

void Waypoint::fromJson(const QJsonValue &jsv)
{
    const auto jso = jsv.toObject();
    for (auto i = jso.begin(); i != jso.end(); ++i) {
        auto f = child(i.key());
        if (f) {
            f->fromJson(i.value());
            continue;
        }
        f = f_actions->child(i.key());
        if (f) {
            f->fromJson(i.value());
            continue;
        }
    }

    // Add feets options
    auto ft = std::round(f_altitude->value().toInt() * M2FT_COEF);
    f_altitude->setOpt("ft", ft);
}

void Waypoint::updateTitle()
{
    if (blockUpdates)
        return;

    QStringList st;
    st.append(QString::number(num() + 1));
    if (f_xtrack->value().toBool())
        st.append("T");
    if (f_atrack->value().toBool())
        st.append("H");
    // st.append(f_altitude->valueText() + f_altitude->units()); // no space between value and units

    // For feets functionality
    if (m_isFeets) {
        QString ftUnits = f_amsl->value().toBool() ? "ft AMSL" : "ft";
        st.append(f_altitude->opts().value("ft", 0).toString() + ftUnits);
    } else {
        st.append(f_altitude->valueText() + f_altitude->units());
    }

    setTitle(st.join(' '));
}

void Waypoint::updateDescr()
{
    if (blockUpdates)
        return;

    setDescr(f_actions->value().toString());
}

void Waypoint::updateAMSL()
{
    if (blockUpdates)
        return;

    // auto m_ref_hmsl = unit()->f_mandala->fact(mandala::est::nav::ref::hmsl::uid);
    // const int href = m_ref_hmsl ? m_ref_hmsl->value().toInt() : 0;

    if (f_amsl->value().toBool()) {
        f_altitude->setUnits(QString("%1 %2").arg(_altUnits, tr("AMSL")));
        // f_altitude->setValue(f_altitude->value().toInt() + href);
    } else {
        f_altitude->setUnits(_altUnits);
        // f_altitude->setValue(f_altitude->value().toInt() - href);
    }
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
    double crs = m_bearing;
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
                crs = prev->bearing();
                wptLine = f_xtrack->value().toBool();
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

    //end bearing
    if (p.path().size() == 2 && crs == m_bearing) {
        crs = p.path().first().azimuthTo(p.path().at(1));
    }
    crs = AppRoot::angle(crs);
    int icrs = (int) (crs / 10) * 10;
    if (m_bearing != icrs) {
        m_bearing = icrs;
        //force next item to be updated
        MissionItem *next = nextItem();
        if (next)
            next->resetPath();
    }
    setBearing(crs);

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

Waypoint::ChosenFact Waypoint::chosen() const
{
    return m_chosen;
}

void Waypoint::setChosen(ChosenFact v)
{
    if (m_chosen == v)
        return;
    m_chosen = v;
    emit chosenChanged();
}

int Waypoint::unsafeAgl() const
{
    return UNSAFE_AGL;
}

void Waypoint::updateAltDescr() {
    if(f_amsl->value().toBool())
        f_altitude->setDescr("Altitude above mean sea level");
    else
        f_altitude->setDescr("Altitude above takeoff point");
}

void Waypoint::calcAltitude()
{
    if (std::isnan(m_elevation))
        return;
    if (m_chosen != AGL)
        return;
   
    auto heightAmsl = m_elevation + f_agl->value().toDouble();
    auto refHmsl = f_refHmsl ? f_refHmsl->value().toDouble() : 0;
    if (f_amsl->value().toBool())
        f_altitude->setValue(heightAmsl);
    else
        f_altitude->setValue((heightAmsl - refHmsl));
}

void Waypoint::recalcAltitude()
{
    auto refHmsl = f_refHmsl ? f_refHmsl->value().toDouble() : 0;
    if (m_isFeets) {
        int ft = f_altitude->opts().value("ft", 0).toInt();
        int refHmslFt = static_cast<int>(refHmsl * M2FT_COEF);
        ft += f_amsl->value().toBool() ? refHmslFt : -refHmslFt;
        f_altitude->setOpt("ft", ft);
    }
    auto alt = f_altitude->value().toDouble();
    alt += f_amsl->value().toBool() ? refHmsl : -refHmsl;
    f_altitude->setValue(alt);
}

void Waypoint::calcAgl()
{
    if (std::isnan(m_elevation)) {
        f_agl->setValue(0);
        return;
    }

    double diff = f_altitude->value().toDouble() - m_elevation;
    auto refHmsl = f_refHmsl ? f_refHmsl->value().toDouble() : 0;
    if (!f_amsl->value().toBool())
        diff += refHmsl;
    f_agl->setValue(diff);
}

void Waypoint::calcAltitudeFt() {
    if(std::isnan(m_elevation))
        return;
    auto refHmsl = f_refHmsl ? f_refHmsl->value().toDouble() : 0;
    int refHmslFt = static_cast<int>(refHmsl * M2FT_COEF);
    int hAmsl = static_cast<int>(f_agl->opts().value("ft", 0).toInt() + m_elevation * M2FT_COEF);
    int ft = f_amsl->value().toBool() ? hAmsl : hAmsl - refHmslFt;
    f_altitude->setOpt("ft", ft);
}
