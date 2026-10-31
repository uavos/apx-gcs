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

#include <QColor>
#include <QGeoCoordinate>
#include <QGeoRectangle>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSharedPointer>
#include <QString>
#include <QVector>

struct KmlPolygon;

// Web Mercator projection to the unit square, the same one the map uses
namespace KmlMercator {
QPointF fromGeo(const QGeoCoordinate &c);
QGeoCoordinate toGeo(const QPointF &p);
QRectF fromGeoRect(const QGeoRectangle &r);
} // namespace KmlMercator

struct KmlZone
{
    QVector<QPointF> outer;          // mercator unit coordinates
    QVector<QVector<QPointF>> holes; // mercator unit coordinates
    QRectF bbox;
    QPointF labelPos;
    QColor color;
    QString name;  // compact identifier for the label, e.g. P175
    QString upper; // formatted upper limit, e.g. (3500)
    QString lower; // formatted lower limit, e.g. GND
};

// Immutable snapshot shared between the GUI thread and the render thread
struct KmlDataSet
{
    QVector<KmlZone> zones;
    QRectF bbox;

    static QSharedPointer<const KmlDataSet> fromPolygons(const QList<KmlPolygon> &polygons);
};

using KmlDataSetPtr = QSharedPointer<const KmlDataSet>;
