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
#include "Geo.h"
#include "MissionField.h"
#include "MissionMapItemsModel.h"
#include "UnitMission.h"

#include <App/App.h>
#include <QGeoCircle>

Geo::Geo(MissionGroup *parent)
    : MissionItem(parent, "g#", "", "")
{
    setOpt("color", "#E65100");

    // geofence role
    f_role = new MissionField(this, "role", tr("Role"), "", Fact::Enum);
    f_role->setEnumStrings({
        "safe", // must reflect xbus::mission::geo_s
        "nofly",
        "terminate",
        "auxiliary",
    });
    connect(f_role, &Fact::valueChanged, this, [this]() {
        // color depends on role
        QString c = "#E65100"; //orange
        switch ((xbus::mission::geo_s::role_e) f_role->value().toInt()) {
        case xbus::mission::geo_s::SAFE: // safe - green
            c = "#1B5E20";
            break;
        case xbus::mission::geo_s::DENY: // no-fly - yellow
            c = "#a0500e";
            break;
        case xbus::mission::geo_s::TERM: // terminate - red
            c = "#D50000";
            break;
        case xbus::mission::geo_s::AUX: // auxiliary - blue
            c = "#2962FF";
            break;
        }
        setOpt("color", c);
    });
    emit f_role->valueChanged();

    // shape
    f_shape = new MissionField(this, "shape", tr("Shape"), "", Fact::Enum);
    f_shape->setEnumStrings({
        "circle", // must reflect xbus::mission::geo_s
        "polygon",
        "line",
    });

    // other fields
    f_label = new MissionField(this, "label", tr("Label"), tr("Geofence label"), Fact::Text);

    f_top = new MissionField(this, "top", tr("Top"), tr("Top altitude AMSL"), Fact::Int);
    f_top->setUnits("m");
    f_top->setMin(0);
    f_top->setIncrement(100);
    f_top->setEnumStrings({
        tr("unlimited"),
    });

    f_bottom = new MissionField(this, "bottom", tr("Bottom"), tr("Bottom altitude AMSL"), Fact::Int);
    f_bottom->setUnits("m");
    f_bottom->setMin(0);
    f_bottom->setIncrement(100);
    f_bottom->setEnumStrings({
        tr("ground"),
    });

    f_inverted = new MissionField(this,
                                  "inverted",
                                  tr("Inverted"),
                                  tr("Valid when outside"),
                                  Fact::Bool);

    // fields specific to shape
    f_radius = new MissionField(this, "radius", tr("Radius"), tr("Geofence radius"), Fact::Float);
    f_radius->setUnits("m");
    f_radius->setMin(100);
    f_radius->setIncrement(100);
    connect(this, &MissionItem::coordinateChanged, this, &Geo::radiusPointChanged);
    connect(f_radius, &Fact::valueChanged, this, &Geo::radiusPointChanged);
    f_radius->setValue(500);

    f_points = new Fact(this,
                        "points",
                        tr("Points"),
                        tr("Geofence points"),
                        Fact::Group | Fact::ModifiedGroup | Fact::Count);
    m_pointsModel = new MissionMapItemsModel(f_points);

    f_p2 = new MissionPoint(this, "p2", tr("Point 2"), tr("Second point of line"));

    // select applicable shape parameters
    connect(f_shape, &Fact::valueChanged, this, [this]() {
        switch ((xbus::mission::geo_s::shape_e) f_shape->value().toInt()) {
        case xbus::mission::geo_s::CIRCLE: // circle
            f_radius->setVisible(true);
            f_points->setVisible(false);
            f_p2->setVisible(false);
            break;
        case xbus::mission::geo_s::POLYGON: // polygon
            f_radius->setVisible(false);
            f_points->setVisible(true);
            f_p2->setVisible(false);
            if (f_points->size() < 3) {
                for (int i = f_points->size(); i < 4; ++i)
                    addPoint(coordinate().atDistanceAndAzimuth(100.0 * (i + 1), 90.0 * i));
            }
            break;
        case xbus::mission::geo_s::LINE: // line
            f_radius->setVisible(false);
            f_points->setVisible(false);
            f_p2->setVisible(true);
            if (!f_p2->coordinate().isValid())
                f_p2->setCoordinate(coordinate().atDistanceAndAzimuth(500.0, 45.0));
            break;
        }
    });
    emit f_shape->valueChanged();

    // track active status when selected
    connect(this, &MissionItem::selectedChanged, this, &Geo::updateActive);

    //title
    updateTitle();
    for (auto f : facts())
        connect(f, &Fact::valueChanged, this, &Geo::updateTitle);

    App::jsync(this);
}

QJsonValue Geo::toJson()
{
    auto jso = MissionItem::toJson().toObject();

    // remove zero altitude limits
    if (f_top->value().toInt() == 0)
        jso.remove(f_top->name());
    if (f_bottom->value().toInt() == 0)
        jso.remove(f_bottom->name());

    // shape specific
    if (f_shape->value().toInt() == xbus::mission::geo_s::POLYGON) {
        // points
        QJsonArray jsa;
        for (auto i : f_points->facts()) {
            auto p = qobject_cast<MissionPoint *>(i);
            if (p)
                jsa.append(QJsonObject{
                    {"lat", p->coordinate().latitude()},
                    {"lon", p->coordinate().longitude()},
                });
        }
        jso["points"] = jsa;
    } else if (f_shape->value().toInt() == xbus::mission::geo_s::LINE) {
        // point 2
        if (f_p2->coordinate().isValid()) {
            jso["p2"] = QJsonObject{
                {"lat", f_p2->coordinate().latitude()},
                {"lon", f_p2->coordinate().longitude()},
            };
        } else {
            jso.remove("p2");
        }
    }

    return jso;
}

void Geo::fromJson(const QJsonValue &jsv)
{
    auto jso = jsv.toObject();
    // points array
    if (jso.contains(f_points->name())) {
        auto jsa = jso.value(f_points->name()).toArray();
        jso.remove(f_points->name());
        f_points->deleteChildren();
        for (auto jv : jsa) {
            auto jso = jv.toObject();
            addPoint(QGeoCoordinate(jso.value("lat").toDouble(), jso.value("lon").toDouble()));
        }
    }

    // p2 point coordinate
    if (jso.contains(f_p2->name())) {
        auto jso2 = jso.value(f_p2->name()).toObject();
        jso.remove(f_p2->name());
        f_p2->setCoordinate(
            QGeoCoordinate(jso2.value("lat").toDouble(), jso2.value("lon").toDouble()));
    }

    MissionItem::fromJson(jso);
}

void Geo::updateTitle()
{
    QStringList st;

    st.append(f_role->valueText().left(3).toUpper());
    st.append("#" + QString::number(num() + 1));

    if (f_inverted->value().toBool()) {
        st.append("n");
    }

    auto label = f_label->valueText();
    if (!label.isEmpty())
        st.append(label);

    auto top = f_top->value().toInt();
    auto bottom = f_bottom->value().toInt();
    if (bottom != 0 && top != 0) {
        st.append(QString("%1-%2").arg(bottom).arg(top));
    } else if (bottom != 0) {
        st.append(QString("%1+").arg(bottom));
    } else if (top != 0) {
        st.append(QString("0-%1").arg(top));
    }

    switch ((xbus::mission::geo_s::shape_e) f_shape->value().toInt()) {
    case xbus::mission::geo_s::CIRCLE:
        st.append(QString("C%1").arg(AppRoot::distanceToString(f_radius->value().toInt())));
        break;
    case xbus::mission::geo_s::LINE:
        st.append(QString("L%1").arg(
            AppRoot::distanceToString((coordinate().distanceTo(f_p2->coordinate())))));
        break;
    case xbus::mission::geo_s::POLYGON:
        st.append(QString("[%1]").arg(f_points->size()));
        break;
    }

    setTitle(st.join(' '));
}

QGeoRectangle Geo::boundingGeoRectangle() const
{
    QGeoRectangle r;
    switch ((xbus::mission::geo_s::shape_e) f_shape->value().toInt()) {
    case xbus::mission::geo_s::CIRCLE: // circle
        r = QGeoCircle(coordinate(), std::abs(f_radius->value().toDouble())).boundingGeoRectangle();
        break;
    case xbus::mission::geo_s::POLYGON: // polygon
        for (auto i : f_points->facts()) {
            auto p = qobject_cast<MissionPoint *>(i);
            if (p)
                r = r.isValid() ? r.united(QGeoRectangle(p->coordinate(), p->coordinate()))
                                : QGeoRectangle(p->coordinate(), p->coordinate());
        }
        break;
    case xbus::mission::geo_s::LINE: // line
        r = QGeoRectangle(coordinate(), f_p2->coordinate());
        break;
    }
    return MissionItem::boundingGeoRectangle().united(r);
}

void Geo::addPoint(QGeoCoordinate c, int n)
{
    if (f_shape->value().toInt() != xbus::mission::geo_s::POLYGON)
        return;

    auto pt = new MissionPoint(f_points, tr("Polygon vertex"), c);
    if (n >= 0)
        pt->move(n, false);

    connect(pt, &Fact::activeChanged, this, &Geo::updateActive);
    connect(pt, &MissionPoint::coordinateChanged, this, &Geo::updatePolygon);
    updatePolygon();
}
void Geo::removePoint(int n)
{
    if (f_shape->value().toInt() != xbus::mission::geo_s::POLYGON)
        return;
    if (n < 0 || n >= f_points->size())
        return;
    if (f_points->size() <= 3)
        return;

    auto pt = qobject_cast<MissionPoint *>(f_points->child(n));
    if (!pt)
        return;

    pt->setActive(false);
    pt->deleteFact();
    updatePolygon();
}

QGeoCoordinate Geo::radiusPoint() const
{
    return f_pos->coordinate().atDistanceAndAzimuth(std::abs(f_radius->value().toInt()), 90.0);
}
void Geo::setRadiusPoint(const QGeoCoordinate &v)
{
    auto p = f_pos->coordinate();
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
        dist = (dist / 1000) * 1000;
    else if (dist > 500)
        dist = (dist / 100) * 100;
    else
        dist = (dist / 10) * 10;
    f_radius->setValue(f_radius->value().toInt() < 0 ? -dist : dist);
}

void Geo::updatePolygon()
{
    QGeoPolygon p;
    for (auto i : f_points->facts()) {
        auto pt = qobject_cast<MissionPoint *>(i);
        if (pt)
            p.addCoordinate(pt->coordinate());
    }
    m_polygon = p;
    emit polygonChanged();
    emit polyHandleChanged();
}

void Geo::updateActive()
{
    // track active status if any handle pints are selected/active
    bool active = false;
    if (selected())
        active = true;
    else if (f_shape->value().toInt() == xbus::mission::geo_s::POLYGON) {
        for (auto i : f_points->facts()) {
            if (i->active()) {
                active = true;
                break;
            }
        }
    } else if (f_shape->value().toInt() == xbus::mission::geo_s::LINE) {
        if (f_pos->active())
            active = true;
    }
    setActive(active);
    emit polyHandleChanged();
}

QGeoCoordinate Geo::getPolyHandle(bool prev) const
{
    // point between prev and current
    do {
        if (f_shape->value().toInt() != xbus::mission::geo_s::POLYGON)
            break;
        if (f_points->size() < 2)
            break;

        // find selected index
        int p1 = -1;
        for (int i = 0; i < f_points->size(); ++i)
            if (f_points->child(i)->active()) {
                p1 = i;
                break;
            }
        if (p1 < 0)
            break;

        // qDebug() << "Handle between" << p1 << (prev ? "prev" : "next");

        // previous point
        int p2 = prev ? (p1 > 0 ? (p1 - 1) : (f_points->size() - 1))
                      : (p1 < (f_points->size() - 1) ? (p1 + 1) : 0);

        auto pt1 = qobject_cast<MissionPoint *>(f_points->child(p1));
        auto pt2 = qobject_cast<MissionPoint *>(f_points->child(p2));
        if (!(pt1 && pt2))
            break;

        auto dist = pt1->coordinate().distanceTo(pt2->coordinate());
        if (dist < 20)
            break;
        auto az = pt1->coordinate().azimuthTo(pt2->coordinate());
        auto c = pt1->coordinate().atDistanceAndAzimuth(dist / 2.0, az);

        // qDebug() << "Handle" << (prev ? "prev" : "next") << c << dist << az;
        return c;

    } while (0);

    // qDebug() << "No handle" << (prev ? "prev" : "next");
    return QGeoCoordinate();
}
