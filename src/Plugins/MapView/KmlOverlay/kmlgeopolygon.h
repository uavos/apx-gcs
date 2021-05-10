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

#include <QGeoPolygon>
#include <QPolygon>
#include <QQuickPaintedItem>

class KmlGeoPolygon : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QGeoPolygon geoPolygon READ getGeoPolygon WRITE setGeoPolygon)
    Q_PROPERTY(QObject *map READ getMap WRITE setMap)
    Q_PROPERTY(QGeoShape area READ getArea WRITE setArea)
    Q_PROPERTY(QColor color READ getColor WRITE setColor NOTIFY colorChanged)
public:
    explicit KmlGeoPolygon(QQuickItem *parent = 0);

    static void registerQmlType();

    void setGeoPolygon(const QGeoPolygon &geoPolygon);
    QGeoPolygon getGeoPolygon() const;

    void setMap(QObject *map);
    QObject *getMap() const;

    QGeoShape getArea() const;
    void setArea(const QGeoShape &area);

    void setColor(const QColor &color);
    QColor getColor() const;

    void paint(QPainter *painter);

private:
    QObject *m_map;
    QGeoShape m_area;
    QGeoPolygon m_geoPolygon;
    QPolygon m_polygon;
    QVector<QPolygon> m_holes;
    QColor m_color;
    QRegion m_clip;

    QPoint fromCoordinate(const QGeoCoordinate &c);

    void clearPolygon();
    void prepareForDrawing();
    void recalcCoordinates();

signals:
    void colorChanged();
};
