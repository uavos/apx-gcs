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
#include "kmlpolygonsmodel.h"

#include <algorithm>
#include <QDebug>
#include <QGeoCoordinate>
#include <QGeoPolygon>

KmlPolygonsModel::KmlPolygonsModel() {}

QPointF KmlPolygonsModel::setPolygons(const QList<KmlPolygon> &kmlPolygons)
{
    m_allPolygons.clear();

    QPolygonF allPoints;
    for (auto &p : kmlPolygons) {
        QPolygonF polygon = toPolygon(p.data);
        allPoints.append(polygon);

        KmlPolygonExtended kmlPolygonExtended;
        kmlPolygonExtended.kmlPolygon = p;
        kmlPolygonExtended.polygon = polygon;
        m_allPolygons.append(kmlPolygonExtended);
    }

    auto center = std::accumulate(allPoints.begin(), allPoints.end(), QPointF(0, 0))
                  / allPoints.size();

    beginResetModel();
    m_viewPolygons.clear();
    endResetModel();
    updateViewPolygons();

    return center;
}

void KmlPolygonsModel::setBoundingBox(const QRectF &bb)
{
    m_bb = QRectF(bb.x(), bb.y(), bb.width(), bb.height());
    updateViewPolygons();
}

int KmlPolygonsModel::rowCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return m_viewPolygons.size();
}

int KmlPolygonsModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return 1;
}

QVariant KmlPolygonsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    int row = index.row();
    if (row >= 0 && row < m_viewPolygons.size()) {
        if (role == Polygon) {
            result = QVariant::fromValue(m_viewPolygons[row].kmlPolygon.data);
        } else if (role == Color) {
            result = m_viewPolygons[row].kmlPolygon.color;
        }
    }
    return result;
}

QHash<int, QByteArray> KmlPolygonsModel::roleNames() const
{
    QHash<int, QByteArray> rolenames = {{Polygon, "polygon"}, {Color, "polygonColor"}};
    return rolenames;
}

void KmlPolygonsModel::updateViewPolygons()
{
    for (int i = 0; i < m_viewPolygons.size(); i++) {
        if (!m_viewPolygons[i].polygon.intersects(m_bb)) {
            beginRemoveRows(QModelIndex(), i, i);
            m_viewPolygons.removeAt(i);
            endRemoveRows();
        }
    }

    for (auto &p : m_allPolygons) {
        if (!m_viewPolygons.contains(p) && p.polygon.intersects(m_bb)) {
            int row = m_viewPolygons.size();
            beginInsertRows(QModelIndex(), row, row);
            m_viewPolygons.append(p);
            endInsertRows();
        }
    }
}

QPolygonF KmlPolygonsModel::toPolygon(const QGeoPolygon &geoPolygon)
{
    QPolygonF polygon;
    for (int i = 0; i < geoPolygon.size(); i++)
        polygon.append(
            QPointF(geoPolygon.coordinateAt(i).latitude(), geoPolygon.coordinateAt(i).longitude()));
    return polygon;
}
