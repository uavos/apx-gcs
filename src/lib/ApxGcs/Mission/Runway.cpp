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
#include "UnitMission.h"
#include <App/App.h>
#include <Fleet/Unit.h>
#include <Nodes/NodeItem.h>
#include <Nodes/Nodes.h>


Runway::Runway(MissionGroup *parent)
    : MissionItem(parent, "r#", "", "")
{
    setOpt("color", "#0A6DBE");

    f_type = new MissionField(this, "type", tr("Type"), tr("Landing pattern type"), Enum);
    f_type->setEnumStrings(QStringList() << "left"
                                         << "right");

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
    connect(this, &Fact::removed, group, [this]() {
        //qDebug()<<"rm";
        if (group->size() <= 0) {
            //qDebug()<<"rst";
            group->mission->setStartPoint(QGeoCoordinate());
            group->mission->setStartLength(0);
            group->mission->setStartHeading(0);
            group->mission->setCoordinate(QGeoCoordinate());
        }
    });

    // landing pattern estimation
    connect(&_taUpdate, &DelayedEvent::triggered, this, &Runway::updateTa);
    connect(this, &Runway::appPointChanged, this, &Runway::scheduleTaUpdate);
    connect(f_type, &Fact::valueChanged, this, &Runway::scheduleTaUpdate);

    // last waypoint signals are connected in updateTa():
    // itemInserted is emitted from the Fact base constructor,
    // when the item is not a MissionItem yet
    auto wp = group->mission->f_wp;
    connect(wp, &Fact::sizeChanged, this, &Runway::scheduleTaUpdate);
    connect(wp, &Fact::itemMoved, this, &Runway::scheduleTaUpdate);
    connect(wp, &Fact::itemInserted, this, &Runway::scheduleTaUpdate);

    connect(unit()->f_nodes, &Nodes::validChanged, this, &Runway::scheduleTaUpdate);
    connect(unit()->f_nodes, &Nodes::nodeNotify, this, &Runway::scheduleTaUpdate);

    scheduleTaUpdate();

    App::jsync(this);
}

void Runway::scheduleTaUpdate()
{
    _taUpdate.schedule();
}

MissionItem *Runway::lastWaypoint() const
{
    auto wp = group->mission->f_wp;
    if (wp->size() <= 0)
        return nullptr;
    return qobject_cast<MissionItem *>(wp->child(wp->size() - 1));
}

Fact *Runway::shivaParam(const QString &path) const
{
    // prefer the node with shiva enabled
    NodeItem *best{};
    for (auto node : unit()->f_nodes->nodes()) {
        auto shiva = node->child("shiva");
        if (!shiva)
            continue;
        auto mode = shiva->child("mode");
        if (mode && mode->value().toUInt() > 0) {
            best = node;
            break;
        }
        if (!best)
            best = node;
    }
    if (!best)
        return nullptr;

    Fact *f = best->child("shiva");
    for (const auto &s : path.split('.')) {
        f = f->child(s);
        if (!f)
            return nullptr;
    }
    return f;
}

Runway::PerfParams Runway::perfParams()
{
    PerfParams p; // apx-fw defaults

    auto read = [this](const QString &path, double &v) {
        auto f = shivaParam(path);
        if (!f)
            return;
        // track config changes
        connect(f, &Fact::valueChanged, this, &Runway::scheduleTaUpdate, Qt::UniqueConnection);
        v = f->value().toDouble();
    };

    double vtol{};
    read("perf.radius", p.radius);
    read("perf.type", vtol);
    p.vtol = vtol > 0;

    // fall back to defaults for unset or invalid values
    const PerfParams def;
    if (p.radius <= 0)
        p.radius = def.radius;
    return p;
}

void Runway::updateTa()
{
    // track the last waypoint
    MissionItem *last = lastWaypoint();
    if (last != m_lastWp) {
        if (m_lastWp)
            disconnect(m_lastWp, nullptr, this, nullptr);
        m_lastWp = last;
        if (last) {
            connect(last, &MissionItem::coordinateChanged, this, &Runway::scheduleTaUpdate);
        }
    }

    const PerfParams p = perfParams();

    // ProcLD::event(evt_cmd_rw): CTOL circle radius is twice the loiter radius
    const double radius = p.vtol ? p.radius : p.radius * 2.0;
    const bool left = f_type->value().toInt() == 0;
    const double hdgTA = heading() + (left ? -90.0 : 90.0);

    QGeoCoordinate center;
    const QGeoCoordinate app = appPoint();
    if (app.isValid())
        center = app.atDistanceAndAzimuth(radius, hdgTA);

    QGeoPath path = estimateTaPath(center, radius);

    if (m_taCenter == center && m_taRadius == radius && m_taPath == path)
        return;
    m_taCenter = center;
    m_taRadius = radius;
    m_taPath = path;
    emit taChanged();
}

QGeoPath Runway::estimateTaPath(const QGeoCoordinate &center, double radius) const
{
    // Flight path after the last waypoint:
    // ProcWPT::_done -> LANDING -> ProcLD_runway::INIT sets the TA circle center
    // as the loiter destination, so the vehicle flies straight towards the center
    // (RegL1::navigate_loiter) until it gets close to the circle.
    // Only this straight segment is estimated, up to the circle edge.

    QGeoPath path;
    MissionItem *last = lastWaypoint();
    if (!last || !center.isValid() || radius <= 0)
        return path;

    const QGeoCoordinate pt = last->coordinate();
    if (!pt.isValid())
        return path;

    if (pt.distanceTo(center) <= radius)
        return path; // already inside the circle

    path.addCoordinate(pt);
    path.addCoordinate(center.atDistanceAndAzimuth(radius, center.azimuthTo(pt)));
    return path;
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
        f_type->setValue(ne.y() > 0 ? "left" : "right");
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
