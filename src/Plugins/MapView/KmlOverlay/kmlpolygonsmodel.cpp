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

#include <QGeoCoordinate>
#include <QGeoPolygon>
#include <QGeoRectangle>

KmlPolygonsModel::KmlPolygonsModel() {}

void KmlPolygonsModel::clearPolygons()
{
    beginResetModel();

    m_allPolygons.clear();
    m_viewPolygons.clear();
    m_centerSum = QPointF();
    m_centerCount = 0;

    endResetModel();
}

void KmlPolygonsModel::appendPolygons(const QList<KmlPolygon> &polygons,
                                     int start,
                                     int count)
{
    const int end = qMin(start + count, polygons.size());

    for (int i = start; i < end; ++i) {
        const auto &polygon = polygons.at(i);
        if (polygon.data.size() < 3)
            continue;

        const QGeoRectangle rect = polygon.data.boundingGeoRectangle();
        if (!rect.isValid())
            continue;

        const double minLat = rect.bottomRight().latitude();
        const double maxLat = rect.topLeft().latitude();
        const double minLon = rect.topLeft().longitude();
        const double maxLon = rect.bottomRight().longitude();

        KmlPolygonExtended item;
        item.kmlPolygon = polygon;
        item.boundingBox = QRectF(QPointF(minLat, minLon),
                                  QPointF(maxLat, maxLon)).normalized();

        m_centerSum += QPointF((minLat + maxLat) / 2.0,
                               (minLon + maxLon) / 2.0);
        ++m_centerCount;

        m_allPolygons.append(item);

        if (!m_hasBoundingBox || item.boundingBox.intersects(m_bb)) {
            const int row = m_viewPolygons.size();
            beginInsertRows(QModelIndex(), row, row);
            m_viewPolygons.append(item);
            endInsertRows();
        }
    }
}

QPointF KmlPolygonsModel::center() const
{
    return m_centerCount > 0 ? m_centerSum / m_centerCount : QPointF();
}

QPointF KmlPolygonsModel::setPolygons(const QList<KmlPolygon> &kmlPolygons)
{
    clearPolygons();
    appendPolygons(kmlPolygons, 0, kmlPolygons.size());
    return center();
}

void KmlPolygonsModel::setBoundingBox(const QRectF &bb)
{
    const QRectF visible = bb.normalized();

    if (!visible.isValid())
        return;

    if (m_hasBoundingBox && m_bb.contains(visible))
        return;

    const double marginX = visible.width() * 0.75;
    const double marginY = visible.height() * 0.75;

    m_bb = visible.adjusted(-marginX, -marginY, marginX, marginY);
    m_hasBoundingBox = true;

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
    const int row = index.row();

    if (row >= 0 && row < m_viewPolygons.size()) {
        const auto &polygon = m_viewPolygons.at(row).kmlPolygon;

        if (role == Polygon) {
            result = QVariant::fromValue(polygon.data);
        } else if (role == Color) {
            result = polygon.color;
        } else if (role == Path) {
            result = QVariant::fromValue(polygon.data.perimeter());
        }
    }

    return result;
}

QHash<int, QByteArray> KmlPolygonsModel::roleNames() const
{
    return {
        {Polygon, "polygon"},
        {Color, "polygonColor"},
        {Path, "polygonPath"}
    };
}

void KmlPolygonsModel::updateViewPolygons()
{
    for (int i = m_viewPolygons.size() - 1; i >= 0; --i) {
        if (m_hasBoundingBox
            && !m_viewPolygons.at(i).boundingBox.intersects(m_bb)) {
            beginRemoveRows(QModelIndex(), i, i);
            m_viewPolygons.removeAt(i);
            endRemoveRows();
        }
    }

    if (!m_hasBoundingBox)
        return;

    for (const auto &p : m_allPolygons) {
        if (!m_viewPolygons.contains(p) && p.boundingBox.intersects(m_bb)) {
            const int row = m_viewPolygons.size();
            beginInsertRows(QModelIndex(), row, row);
            m_viewPolygons.append(p);
            endInsertRows();
        }
    }
}
