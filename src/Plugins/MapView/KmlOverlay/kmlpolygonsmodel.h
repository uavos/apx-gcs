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

#include "kmlparser.h"
#include <QAbstractListModel>
#include <QPolygonF>

class KmlPolygonsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { Polygon = Qt::UserRole + 1, Color };
    KmlPolygonsModel();

    QPointF setPolygons(const QList<KmlPolygon> &kmlPolygons);
    void setBoundingBox(const QRectF &bb);

    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct KmlPolygonExtended
    {
        KmlPolygon kmlPolygon;
        QPolygonF polygon;
        bool operator==(const KmlPolygonExtended &other) const
        {
            return kmlPolygon.id == other.kmlPolygon.id;
        }
    };

    QPolygonF m_bb;
    QList<KmlPolygonExtended> m_allPolygons;
    QList<KmlPolygonExtended> m_viewPolygons;

    void updateViewPolygons();

    QPolygonF toPolygon(const QGeoPolygon &geoPolygon);
};
