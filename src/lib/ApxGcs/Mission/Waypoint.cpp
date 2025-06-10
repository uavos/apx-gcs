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

#include <QFuture>
#include <QtConcurrent>

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

    // Actions
    f_actions = new WaypointActions(this);

    // Correct rout for elevationmap plugin
    f_correct = new MissionField(this,
                                 "correct",
                                 tr("Path correction"),
                                 tr("Correct unsafe path's points"),
                                 CloseOnTrigger);
    f_correct->setVisible(false);

    // Default values
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

    connect(this, &MissionItem::isFeetsChanged, this, &Waypoint::updateTitle);
    connect(f_altitude, &Fact::optsChanged, this, &Waypoint::updateTitle);
    connect(f_altitude, &Fact::optsChanged, this, &Waypoint::processAglFt);
    connect(f_agl, &Fact::optsChanged, this, [this]() {if (this->chosen() == AGL) calcAltitudeFt();});

    // Elevation map and agl
    connect(f_altitude, &Fact::valueChanged, this, [this]() { if (this->chosen() == ALT) processAgl();});
    connect(f_altitude, &Fact::triggered, this, [this]() { this->setChosen(ALT); });
    connect(f_agl, &Fact::valueChanged, this, &Waypoint::calcAltitude);
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

    connect(f_correct, &Fact::triggered, this, [this](){ correctPath(); });

    App::jsync(this);
}

void Waypoint::initElevationMap()
{
    f_elevationmap = AppSettings::instance()->findChild("application.plugins.elevationmap");
    if(!f_elevationmap)
        return;
        
    auto mission = group->mission;
    auto order = f_order->value().toInt();
    connect(mission, &UnitMission::startElevationChanged, this, &Waypoint::updateAgl, Qt::UniqueConnection);
    if (order == 1) {
        connect(mission, &UnitMission::startElevationChanged, this, &Waypoint::checkCollision, Qt::UniqueConnection);
    }

    connect(f_amsl, &Fact::valueChanged, this, &Waypoint::recalcAltitude, Qt::UniqueConnection);
    connect(f_amsl, &Fact::valueChanged, this, &Waypoint::updateAgl, Qt::UniqueConnection);
    connect(f_altitude, &Fact::valueChanged, this, &Waypoint::updateMinMaxHeight, Qt::UniqueConnection);
    connect(f_agl, &Fact::valueChanged, this, &Waypoint::checkCollision, Qt::UniqueConnection);

    connect(this, &MissionItem::elevationChanged, this, &Waypoint::updateAgl, Qt::UniqueConnection);
    connect(this, &MissionItem::elevationChanged, this, &Waypoint::setAglEnabled, Qt::UniqueConnection);

    m_timer.setSingleShot(true);
    m_timer.setInterval(TIMEOUT);
    connect(this, &MissionItem::coordinateChanged, this, &Waypoint::startTimer, Qt::UniqueConnection);
    connect(&m_timer, &QTimer::timeout, this, &Waypoint::sendElevationRequest, Qt::UniqueConnection);

    m_geoPathTimer.setSingleShot(true);
    m_geoPathTimer.setInterval(TIMEOUT);
    connect(this, SIGNAL(geoPathChanged()), &m_geoPathTimer, SLOT(start()), Qt::UniqueConnection);
    connect(&m_geoPathTimer, &QTimer::timeout, this, &Waypoint::sendTerrainProfileRequest, Qt::UniqueConnection);

    connect(this, &Waypoint::minHeightChanged, mission, &UnitMission::updateMinHeight, Qt::UniqueConnection);
    connect(this, &Waypoint::maxHeightChanged, mission, &UnitMission::updateMaxHeight, Qt::UniqueConnection);
    connect(this, &Waypoint::collisionChanged, mission, &UnitMission::checkCollision, Qt::UniqueConnection);
    
    Waypoint *prevWp = static_cast<Waypoint *>(prevItem());
    if(prevWp) {
        Fact *prevAltitude = prevWp->f_altitude;
        connect(prevAltitude, &Fact::valueChanged, this, &Waypoint::checkCollision, Qt::UniqueConnection);
    }
    connect(&m_watcher, &QFutureWatcher<TerrainInfo>::finished, this, &Waypoint::updateTerrainInfo, Qt::UniqueConnection);
    connect(App::instance(), &App::appQuit, &m_watcher, &QFutureWatcher<TerrainInfo>::cancel, Qt::UniqueConnection);

    connect(&m_pointsWatcher,
            &QFutureWatcher<QList<QGeoCoordinate>>::finished,
            this,
            &Waypoint::insertNewPoints,
            Qt::UniqueConnection);
    connect(App::instance(),
            &App::appQuit,
            &m_pointsWatcher,
            &QFutureWatcher<QList<QGeoCoordinate>>::cancel,
            Qt::UniqueConnection);

    updateMinMaxHeight();
    updateAgl();
}

QJsonValue Waypoint::toJson()
{
    auto jso = MissionItem::toJson().toObject();

    // Remove agl from oblect
    jso.remove(f_agl->name());

    // Move all actions to object
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

    // Feets functionality
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
    } else {
        f_altitude->setUnits(_altUnits);
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

double Waypoint::minHeight() const
{
    return m_minHeight;
}
void Waypoint::setMinHeight(const double v)
{
    if(m_minHeight == v)
        return;
    m_minHeight = v;
    emit minHeightChanged();
}

double Waypoint::maxHeight() const
{
    return m_maxHeight;
}
void Waypoint::setMaxHeight(const double v)
{
    if(m_maxHeight == v)
        return;
    m_maxHeight = v;
    emit maxHeightChanged();
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

bool Waypoint::collision() const
{
    return m_collision;
}
void Waypoint::setCollision(bool v){
    if(m_collision == v)
        return;
    m_collision = v;
    emit collisionChanged();
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
    auto startHmsl = getStartHMSL();
    if (f_amsl->value().toBool())
        f_altitude->setValue(heightAmsl);
    else
        f_altitude->setValue((heightAmsl - startHmsl));
}

void Waypoint::recalcAltitude()
{
    auto startHmsl = getStartHMSL();
    if (m_isFeets) {
        int ft = f_altitude->opts().value("ft", 0).toInt();
        int startHmslFt = static_cast<int>(startHmsl * M2FT_COEF);
        ft += f_amsl->value().toBool() ? startHmslFt : -startHmslFt;
        f_altitude->setOpt("ft", ft);
    }
    auto alt = f_altitude->value().toDouble();
    alt += f_amsl->value().toBool() ? startHmsl : -startHmsl;
    f_altitude->setValue(alt);
}

void Waypoint::processAgl()
{
    if (std::isnan(m_elevation)) {
        f_agl->setValue(0);
        return;
    }

    calcAgl();
}

void Waypoint::calcAgl()
{
    int diff = f_altitude->value().toInt() - static_cast<int>(m_elevation);
    auto startHmsl = getStartHMSL();
    if (!f_amsl->value().toBool())
        diff += startHmsl;
    f_agl->setValue(diff);
}


// Feets processing
void Waypoint::calcAltitudeFt() {
    if (std::isnan(m_elevation))
        return;

    auto startHmsl = getStartHMSL();
    int startHmslFt = static_cast<int>(startHmsl * M2FT_COEF);
    int hAmsl = static_cast<int>(f_agl->opts().value("ft", 0).toInt() + std::round(m_elevation * M2FT_COEF));
    int ft = f_amsl->value().toBool() ? hAmsl : hAmsl - startHmslFt;
    f_altitude->setOpt("ft", ft);
}

void Waypoint::processAglFt()
{
    if (chosen() == AGL)
        return;

    if(std::isnan(m_elevation)) {
        f_agl->setOpt("ft", 0);
        return;
    }

    calcAglFt();
}

void Waypoint::calcAglFt()
{
    auto startHmsl = getStartHMSL();
    int startHmslFt = static_cast<int>(startHmsl * M2FT_COEF);
    int diff = static_cast<int>(f_altitude->opts().value("ft", 0).toInt() - std::round(m_elevation * M2FT_COEF));
    int ft = f_amsl->value().toBool() ? diff : startHmslFt + diff;
    f_agl->setOpt("ft", ft);
}

void Waypoint::updateAgl()
{
    processAgl();
    processAglFt();
}

void Waypoint::setAglEnabled()
{
    f_agl->setEnabled(!std::isnan(m_elevation));
}

void Waypoint::sendTerrainProfileRequest()
{
    emit requestTerrainProfile(m_geoPath);
}

void Waypoint::buildTerrainProfile(const QGeoPath &path)
{
    auto end = m_geoPath.size() - 1;
    auto first = m_geoPath.coordinateAt(0);
    auto last = m_geoPath.coordinateAt(end);

    auto inEnd = path.size() - 1;
    auto firstIn = path.coordinateAt(0);
    auto lastIn = path.coordinateAt(inEnd);

    first.setAltitude(0);
    last.setAltitude(0);
    firstIn.setAltitude(0);
    lastIn.setAltitude(0);

    if (first != firstIn || last != lastIn)
        return;

    if(m_watcher.isRunning())
       m_watcher.cancel();

    clearTerrainProfile();

    if (m_geoPath == path) {
        setCollision(false);
        return;
    }

    QFuture<TerrainInfo> future;
    future = QtConcurrent::run(createTerrainInfo, path);
    m_watcher.setFuture(future);
}

void Waypoint::createTerrainInfo(QPromise<TerrainInfo> &promise, const QGeoPath &path)
{
    QPointF pt;
    double ptDistance{0};
    double ptElevation{0};
    QGeoCoordinate current;
    QGeoCoordinate next;
    TerrainInfo info;
    info.terrainProfile = {};
    info.minHeight = 0;
    info.maxHeight = 0;

    auto lastIndex = path.size() - 1;
    for (qsizetype i = 0; i < lastIndex; ++i) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            return;
        }
        current = path.coordinateAt(i);
        next = path.coordinateAt(i + 1);
        ptElevation = current.altitude();
        info.terrainProfile.append(QPointF(ptDistance, ptElevation));
        ptDistance += current.distanceTo(next);
        if (qIsNaN(ptDistance))
            continue;
        info.minHeight = qMin(info.minHeight, ptElevation);
        info.maxHeight = qMax(info.maxHeight, ptElevation);
    }
    ptElevation = path.coordinateAt(lastIndex).altitude();
    info.terrainProfilePath = path;
    info.terrainProfile.append(QPointF(ptDistance, ptElevation));
    info.minHeight = qMin(info.minHeight, ptElevation);
    info.maxHeight = qMax(info.maxHeight, ptElevation);
    promise.addResult(info);
}

void Waypoint::checkCollision()
{
    if (m_terrainProfile.empty()) {
        setCollision(false);
        return;
    }

    double prevAlt{0};
    auto startHmsl = getStartHMSL();
    Waypoint *prevWp = static_cast<Waypoint *>(prevItem());
    
    // Checking the first point
    if (prevWp) {
        prevAlt = prevWp->f_altitude->value().toInt();
        auto prevAmsl = prevWp->f_amsl->value().toBool();
        if (!prevAmsl) {
            prevAlt += startHmsl;
        }
    } else {
        prevAlt = startHmsl !=0 ? startHmsl : m_terrainProfile.first().y();
    }

    // Checking points on top of each other
    auto dst = m_terrainProfile.last().x();
    if (dst == 0) {
        bool prevCollision = prevWp ? (prevWp->f_agl->value().toInt() < UNSAFE_AGL) : false;
        bool currentCollision = (f_agl->value().toInt() < UNSAFE_AGL);
        bool collision = currentCollision && prevCollision;
        setCollision(collision);
        return;
    }

    auto alt = f_altitude->value().toInt();
    auto amsl = f_amsl->value().toBool();
    if (!amsl)
        alt += startHmsl;

    double tan = static_cast<double>((alt - prevAlt) / dst);
    for (const auto &tp : m_terrainProfile) {
        double k = !prevWp ? (tp.x() / dst): 1; // proportional increase in safe AGL for the first point
        auto safeHeight = tp.y() + UNSAFE_AGL * k;
        auto routeHeight = prevAlt + tp.x() * tan;
        auto diff = std::abs(safeHeight - routeHeight);
        if (routeHeight < safeHeight && ALT_EPS < diff) {
            setCollision(true);
            return;
        }
    }
    setCollision(false);
}

double Waypoint::getStartHMSL()
{
    return group->mission->startElevation();
}

void Waypoint::updateMinMaxHeight() 
{
    bool amsl = f_amsl->value().toBool();
    double alt = f_altitude->value().toDouble();
    if(!amsl)
        alt += getStartHMSL();
    auto minHeight = !qIsNaN(alt) ? qMin(m_terrainProfileMin, alt) : m_terrainProfileMin;
    auto maxHeight = !qIsNaN(alt) ? qMax(m_terrainProfileMax, alt) : m_terrainProfileMax;
    setMinHeight(minHeight);
    setMaxHeight(maxHeight);
}

void Waypoint::updateTerrainInfo()
{
    auto result = m_watcher.result();
    // Rebuild GeoPath
    m_terrainProfilePath = result.terrainProfilePath;
    m_terrainProfileMin = result.minHeight;
    m_terrainProfileMax = result.maxHeight;
    updateMinMaxHeight();
    setTerrainProfile(result.terrainProfile);
    checkCollision();
}

// Waypoint path correction
void Waypoint::correctPath(bool reply)
{
    if (m_reply != reply)
        m_reply = reply;

    if (!m_collision) {
        if(!m_reply)
            return;
        auto index = indexInParent();
        emit responseCorrectPath(QList<QGeoCoordinate>(), index);
        return;
    }

    // Correct point altitude 
    auto alt = f_altitude->value().toInt();
    auto agl = f_agl->value().toInt();
    if (agl < UNSAFE_AGL) {
        alt += UNSAFE_AGL - agl;
        f_altitude->setValue(alt);
    }

    Waypoint *prevWp = static_cast<Waypoint *>(prevItem());
    if (!prevWp) {
        if (!m_reply)
            return;
        auto index = indexInParent();      
        emit responseCorrectPath(QList<QGeoCoordinate>(), index);
        return;
    }

    auto amsl = f_amsl->value().toBool();
    if(!amsl)
        alt += getStartHMSL();

    // Correct previous point altitude
    auto prevAlt = prevWp->f_altitude->value().toInt();
    auto prevAgl = prevWp->f_agl->value().toInt();
    if (prevAgl < UNSAFE_AGL) {
        prevAlt += UNSAFE_AGL - prevAgl;
        prevWp->f_altitude->setValue(prevAlt);
    }

    auto prevAmsl = f_amsl->value().toBool();
    if (!prevAmsl)
        prevAlt += getStartHMSL();

    QFuture<QList<QGeoCoordinate>> future;
    future = QtConcurrent::run(getCorrectRoutePoints, m_terrainProfilePath, prevAlt, alt);
    m_pointsWatcher.setFuture(future);
}

void Waypoint::insertNewPoints()
{
    QList<QGeoCoordinate> result = m_pointsWatcher.result();
    if(m_reply) {
        m_reply = false; 
        auto index = indexInParent();
        emit responseCorrectPath(result, index);
        return;
    }

    int wpIndex = indexInParent();
    Waypoint *prevWp = static_cast<Waypoint *>(prevItem());
    auto prevCoordinate = prevWp->coordinate();

    for (int i = result.size() - 1; i >= 0; i--) {
        auto point = result[i];
        int wpHmsl = std::ceil(point.altitude());
        if(i == 0) {
            // Check if first point equal prev waypoint
            auto latDiff = std::abs(result[i].latitude() - prevCoordinate.latitude());
            auto lonDiff = std::abs(result[i].longitude() - prevCoordinate.longitude());
            if (latDiff <= DBL_EPSILON && lonDiff <= DBL_EPSILON) {
                auto prevAmsl = prevWp->f_amsl->value().toBool();
                prevWp->f_amsl->setValue(true);
                prevWp->f_altitude->setValue(wpHmsl);
                prevWp->f_amsl->setValue(prevAmsl);
                return;
            }
        }

        Waypoint *wp = static_cast<Waypoint *>(group->insertObject(point, wpIndex));
        wp->f_amsl->setValue(true);
        wp->f_altitude->setValue(wpHmsl);
        wp->f_atrack->setValue(f_atrack->value());
        wp->f_xtrack->setValue(f_xtrack->value());

        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

void Waypoint::getCorrectRoutePoints(QPromise<QList<QGeoCoordinate>> &promise,
                                                      const QGeoPath &path,
                                                      int hFirst,
                                                      int hLast)
{
    int count{0};
    int pathSize = path.size();
    bool hasCollision = true;
    QList<int> indexes{0, pathSize - 1};

    // Start build terrain profile
    while (hasCollision && count < pathSize) {
        hasCollision = false;
        QList<int> tmp;
        for (int i = 0; i < indexes.size() - 1; i++) {
            // Get coordinates from path
            auto begin = indexes[i];
            auto end = indexes[i + 1];
            auto first = path.coordinateAt(begin);
            auto last = path.coordinateAt(end);

            // Get points height and distance
            double h1 = begin != 0 ? (first.altitude() + UNSAFE_AGL) : hFirst;
            double h2 = end != (pathSize - 1) ? (last.altitude() + UNSAFE_AGL) : hLast;
            auto dst = path.length(begin, end);
            auto tan = static_cast<double>(h2 - h1) / dst;

            // Check intermediate points
            int unsafeIndex{-1};
            double unsafeHeightDiff{0};
            for (int j = begin + 1; j < end; j++) {
                auto coordinate = path.coordinateAt(j);
                auto pointElevation = coordinate.altitude();
                auto routeHeight = h1 + path.length(begin, j) * tan;
                auto safeHeight = pointElevation + UNSAFE_AGL;

                // Check safe AGL
                double diff = std::abs(safeHeight - routeHeight);
                if (routeHeight < safeHeight && ALT_EPS < diff) {
                    hasCollision = true;
                    if (unsafeHeightDiff < diff) {
                        unsafeHeightDiff = diff;
                        unsafeIndex = j;
                    }
                }
            }
            if (unsafeIndex >= 0)
                tmp.append(unsafeIndex);
        }
        indexes.append(tmp);
        std::sort(indexes.begin(), indexes.end());
        count++;
    }

    // Find the first point of a straight section of a path
    int linesFirstIndex{-1};
    const double epsAz = 0.001;
    auto lastPoint = path.coordinateAt(pathSize - 1);
    for (int i = 0; i < path.size() - 1; i++) {
        auto point = path.coordinateAt(i);
        auto nextPoint = path.coordinateAt(i + 1);
        auto az = point.azimuthTo(lastPoint);
        auto nextAz = nextPoint.azimuthTo(lastPoint);
        if(linesFirstIndex >= 0)
            break;
        if (std::abs(az - nextAz) < epsAz)
            linesFirstIndex = i + 1;
    }

    QList<QGeoCoordinate> newPoints;
    
    // Check points less lineFirstIndex
    auto alt4Correct = path.coordinateAt(linesFirstIndex).altitude();
    for (int i = 1; i < indexes.size() - 1; i++) {
        if (linesFirstIndex <= 0)
            break;

        // If the point belongs to a straight section
        if (linesFirstIndex < indexes[i])
            continue;

        // First point for altitude correction append
        // and max terrain elevation on the interval
        if (newPoints.size() == 0) {
            newPoints.append(path.coordinateAt(0));
            for (int j = 0; j < linesFirstIndex; j++) {
                auto alt = path.coordinateAt(j).altitude();
                alt4Correct = std::max(alt4Correct, alt);
            }
        }

        // Correct first point altitude if needed
        if (indexes[i] < linesFirstIndex && linesFirstIndex < indexes[i + 1]) {
            auto alt1 = path.coordinateAt(indexes[i]).altitude();
            auto alt2 = path.coordinateAt(indexes[i+1]).altitude();
            auto dst = path.length(indexes[i], indexes[i + 1]);
            auto indexDst = path.length(indexes[i], linesFirstIndex);
            auto altCorrection = alt1 + (alt2 - alt1) * indexDst / dst;
            alt4Correct = std::max(alt4Correct, altCorrection);
        }
    }

    // Add second point for correction
    if (newPoints.size() != 0) {
        auto point = path.coordinateAt(linesFirstIndex);
        alt4Correct += UNSAFE_AGL;
        point.setAltitude(alt4Correct);
        newPoints.append(point);
        if (hFirst < alt4Correct) {
            newPoints[0].setAltitude(alt4Correct);
        } else {
            newPoints[0].setAltitude(hFirst);
        }
    }

    // Add new points
    for (int i = 1; i < indexes.size() - 1; i++) {
        if (indexes[i] <= linesFirstIndex)
            continue;
        auto point = path.coordinateAt(indexes[i]);
        auto newAlt = point.altitude() + UNSAFE_AGL;
        point.setAltitude(newAlt);
        newPoints.append(point);
    }

    promise.addResult(newPoints);
}
