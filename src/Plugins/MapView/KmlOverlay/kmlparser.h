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
#pragma once

#include <functional>
#include <QColor>
#include <QDomDocument>
#include <QGeoPolygon>

struct KmlPolygon
{
    uint64_t id;
    QColor color;
    QGeoPolygon data;
};

class KmlParser
{
public:
    KmlParser();
    void parse(const QByteArray &data);
    QList<KmlPolygon> getPolygons();

private:
    using IterateCallback = std::function<void(const QDomElement &)>;

    QList<KmlPolygon> m_polygons;
    QDomDocument m_dom;
    uint64_t m_polygonId;

    void iterateOverChildrenElements(const QDomElement &parent,
                                     const QString &tagname,
                                     const IterateCallback &cb);

    void placemarkCallback(const QDomElement &el);
    void polygonCallback(const QDomElement &el, const QColor &color);
    void polygonOuterCallback(const QDomElement &el, KmlPolygon &polygon);
    void polygonInnerCallback(const QDomElement &el, KmlPolygon &polygon);
    void polygonCoordinatesCallback(const QDomElement &el, KmlPolygon &polygon);
    void polygonHolesCallback(const QDomElement &el, KmlPolygon &polygon);
    QList<QGeoCoordinate> parseCoordinates(const QString &text);
};
