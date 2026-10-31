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

    connect(unit()->f_nodes, &Nodes::validChanged, this, &Runway::scheduleTaUpdate);

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
    auto nodes = unit()->f_nodes;
    if (!nodes)
        return nullptr;

    // prefer node with shiva enabled
    NodeItem *best{};
    for (auto node : nodes->nodes()) {
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
    // defaults when node config is not available (apx-fw defaults)
    PerfParams p{200.0, 30.0, 25.0, 0.9, 30.0, false};

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
    read("perf.VC", p.speed);
    read("bearing.dist", p.l1_period);
    read("bearing.damp", p.l1_damp);
    read("roll.cmd.max", p.roll_max);
    read("perf.type", vtol);
    p.vtol = vtol > 0;

    if (p.radius <= 0)
        p.radius = 200.0;
    if (p.speed <= 0)
        p.speed = 30.0;
    if (p.l1_period <= 0)
        p.l1_period = 25.0;
    if (p.l1_damp <= 0)
        p.l1_damp = 0.9;
    if (p.roll_max <= 0 || p.roll_max >= 90)
        p.roll_max = 30.0;
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
            connect(last, &MissionItem::bearingChanged, this, &Runway::scheduleTaUpdate);
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

    QGeoPath path = estimateTaPath(center, radius, left, p);

    if (m_taCenter == center && m_taRadius == radius && m_taPath == path)
        return;
    m_taCenter = center;
    m_taRadius = radius;
    m_taPath = path;
    emit taChanged();
}

QGeoPath Runway::estimateTaPath(const QGeoCoordinate &center,
                                double radius,
                                bool left,
                                const PerfParams &p) const
{
    // Estimates the flight path after the last waypoint:
    // ProcWPT::_done -> LANDING -> ProcLD_runway::INIT (loiter to TA circle)
    // -> TA/APP (circle to approach point) -> FINAL (straight to runway).
    // Guidance mimics RegL1::navigate_loiter: fly towards the circle center,
    // blend into the tangent when closer than L1 distance.

    QGeoPath path;
    MissionItem *last = lastWaypoint();
    if (!last || !center.isValid() || radius <= 0)
        return path;

    QGeoCoordinate pt = last->coordinate();
    if (!pt.isValid())
        return path;

    const double spd = p.speed;
    const double dt = 0.5;
    const double step = dt * spd;
    // turn radius is limited by the roll command limit (RegRoll, shiva.roll.cmd.max),
    // perf.radius is the loiter radius and only bounds the estimate from above
    const double turnRadius = std::min(p.radius,
                                       spd * spd
                                           / (9.81 * std::tan(qDegreesToRadians(p.roll_max))));
    const double turnStep = qRadiansToDegrees(spd / turnRadius) * dt; // max heading change per step
    const double L1 = std::max(10.0, p.l1_damp * p.l1_period / M_PI * spd);
    const bool cw = !left; // right pattern: m.radius > 0 -> clockwise

    // capture tolerance: the discrete model keeps a small steady state offset
    // from the circle (about L1*step/(2*radius)), the rest is blended on the arc
    const double captureTol = std::max(radius * 0.01, step);

    double crs = last->bearing();
    double turnCnt = 0;
    bool captured = false;

    path.addCoordinate(pt);

    for (int i = 0; i < 5000; ++i) {
        const double dist = pt.distanceTo(center);
        const double err = dist - radius;
        const double azc = pt.azimuthTo(center);
        const double tangent = azc + (cw ? -90.0 : 90.0);

        // RegL1: eta1 = asin(xtrack_err / L1), limited to +-90deg
        const double eta1 = qRadiansToDegrees(std::asin(AppRoot::limit(err / L1, -1.0, 1.0)));
        const double desired = tangent + (cw ? eta1 : -eta1);
        double deltaHdg = AppRoot::angle(desired - crs);

        if (std::abs(err) < captureTol && std::abs(AppRoot::angle(tangent - crs)) < 10.0) {
            captured = true;
            break;
        }

        double d = step;
        if (std::abs(deltaHdg) < 10.0 && err > (L1 * 2.0)) {
            // course ok and far away - jump towards the circle
            crs += deltaHdg;
            deltaHdg = 0;
            d = std::min(err - L1, 10.0e+3);
        }

        // update the heading first, then move: the position step with the
        // previous heading lags on a curved path and never converges to the circle
        const double turn = AppRoot::limit(deltaHdg, -turnStep, turnStep);
        crs += turn;
        turnCnt += turn;

        pt = pt.atDistanceAndAzimuth(d, crs);
        path.addCoordinate(pt);

        if (std::abs(turnCnt) > (360.0 * 2.0))
            return QGeoPath(); // can't capture the circle
    }

    if (!captured)
        return QGeoPath();

    // follow the circle to the approach point (ProcLD_runway::APP -> FINAL),
    // blend the remaining offset from the circle smoothly along the arc
    const double hdgTA = heading() + (left ? -90.0 : 90.0);
    const double aApp = hdgTA + 180.0; // azimuth from center to approach point
    const double a0 = center.azimuthTo(pt);
    const double err0 = pt.distanceTo(center) - radius;
    const double sweep = AppRoot::angle360(cw ? (aApp - a0) : (a0 - aApp));
    const double sweepLen = radius * qDegreesToRadians(sweep);
    const double blendLen = std::min(L1, sweepLen / 3.0); // complete the blend before the approach point
    const double stepA = 5.0;
    for (double a = stepA; (a + 1.0) < sweep; a += stepA) {
        const double s = radius * qDegreesToRadians(a); // arc length from the capture point
        const double r = radius + err0 * std::exp(-s / blendLen);
        path.addCoordinate(center.atDistanceAndAzimuth(r, cw ? (a0 + a) : (a0 - a)));
    }
    path.addCoordinate(appPoint());

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
