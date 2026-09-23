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
#include "TerrainProfileItem.h"

#include <Mission/MissionItem.h>

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>

#include <algorithm>
#include <cmath>

namespace {

// root node: child 0 = fill (triangle strip), child 1 = border band (triangle strip)
class ProfileNode : public QSGNode
{
public:
    ProfileNode()
    {
        for (auto *&n : nodes) {
            n = new QSGGeometryNode;
            auto *g = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
            g->setDrawingMode(QSGGeometry::DrawTriangleStrip);
            n->setGeometry(g);
            n->setFlag(QSGNode::OwnsGeometry);
            auto *m = new QSGFlatColorMaterial;
            n->setMaterial(m);
            n->setFlag(QSGNode::OwnsMaterial);
            appendChildNode(n);
        }
    }
    QSGGeometryNode *fill() { return nodes[0]; }
    QSGGeometryNode *line() { return nodes[1]; }

private:
    QSGGeometryNode *nodes[2]{nullptr, nullptr};
};

} // namespace

TerrainProfileItem::TerrainProfileItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

QObject *TerrainProfileItem::missionItem() const
{
    return m_item;
}

void TerrainProfileItem::setMissionItem(QObject *v)
{
    auto item = qobject_cast<MissionItem *>(v);
    if (m_item == item)
        return;
    if (m_item)
        disconnect(m_item, nullptr, this, nullptr);
    m_item = item;
    if (m_item)
        connect(m_item,
                &MissionItem::terrainProfileChanged,
                this,
                &TerrainProfileItem::reloadProfile);
    emit missionItemChanged();
    reloadProfile();
}

void TerrainProfileItem::reloadProfile()
{
    const auto count = m_profile.size();
    m_profile = m_item ? m_item->terrainProfile() : QList<QPointF>();
    if (count != m_profile.size())
        emit pointCountChanged();
    markGeometryDirty();
}

void TerrainProfileItem::setXOffset(double v)
{
    if (qFuzzyCompare(m_xOffset, v))
        return;
    m_xOffset = v;
    emit xOffsetChanged();
    markGeometryDirty();
}

void TerrainProfileItem::setViewStart(double v)
{
    if (qFuzzyCompare(m_viewStart, v))
        return;
    m_viewStart = v;
    emit viewStartChanged();
    markGeometryDirty();
}

void TerrainProfileItem::setViewSpan(double v)
{
    if (qFuzzyCompare(m_viewSpan, v))
        return;
    m_viewSpan = v;
    emit viewSpanChanged();
    markGeometryDirty();
}

void TerrainProfileItem::setMinHeight(double v)
{
    if (qFuzzyCompare(m_minHeight, v))
        return;
    m_minHeight = v;
    emit minHeightChanged();
    markGeometryDirty();
}

void TerrainProfileItem::setMaxHeight(double v)
{
    if (qFuzzyCompare(m_maxHeight, v))
        return;
    m_maxHeight = v;
    emit maxHeightChanged();
    markGeometryDirty();
}

void TerrainProfileItem::setFillColor(const QColor &v)
{
    if (m_fillColor == v)
        return;
    m_fillColor = v;
    emit fillColorChanged();
    m_materialDirty = true;
    update();
}

void TerrainProfileItem::setLineColor(const QColor &v)
{
    if (m_lineColor == v)
        return;
    m_lineColor = v;
    emit lineColorChanged();
    m_materialDirty = true;
    update();
}

void TerrainProfileItem::setLineWidth(double v)
{
    if (qFuzzyCompare(m_lineWidth, v))
        return;
    m_lineWidth = v;
    emit lineWidthChanged();
    markGeometryDirty();
}

void TerrainProfileItem::markGeometryDirty()
{
    m_geometryDirty = true;
    update();
}

void TerrainProfileItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        markGeometryDirty();
}

QSGNode *TerrainProfileItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *node = static_cast<ProfileNode *>(oldNode);
    if (!node) {
        node = new ProfileNode;
        m_geometryDirty = true;
        m_materialDirty = true;
    }

    if (m_materialDirty) {
        static_cast<QSGFlatColorMaterial *>(node->fill()->material())->setColor(m_fillColor);
        static_cast<QSGFlatColorMaterial *>(node->line()->material())->setColor(m_lineColor);
        node->fill()->markDirty(QSGNode::DirtyMaterial);
        node->line()->markDirty(QSGNode::DirtyMaterial);
        m_materialDirty = false;
    }

    if (!m_geometryDirty)
        return node;
    m_geometryDirty = false;

    QSGGeometry *fillGeometry = node->fill()->geometry();
    QSGGeometry *lineGeometry = node->line()->geometry();

    const double w = width();
    const double h = height();
    const double hRange = m_maxHeight - m_minHeight;
    if (m_profile.size() < 2 || w <= 0 || h <= 0 || m_viewSpan <= 0 || hRange <= 0) {
        fillGeometry->allocate(0);
        lineGeometry->allocate(0);
        node->fill()->markDirty(QSGNode::DirtyGeometry);
        node->line()->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

    const double sx = w / m_viewSpan;
    const double sy = h / hRange;
    auto px = [&](double d) { return (d + m_xOffset - m_viewStart) * sx; };
    auto py = [&](double e) { return h - (e - m_minHeight) * sy; };

    // visible part of the profile (distances are monotonic), one extra point at each side
    const double dFrom = m_viewStart - m_xOffset;
    const double dTo = m_viewStart + m_viewSpan - m_xOffset;
    auto lower = std::lower_bound(m_profile.cbegin(),
                                  m_profile.cend(),
                                  dFrom,
                                  [](const QPointF &p, double v) { return p.x() < v; });
    auto upper = std::upper_bound(m_profile.cbegin(),
                                  m_profile.cend(),
                                  dTo,
                                  [](double v, const QPointF &p) { return v < p.x(); });
    qsizetype first = std::distance(m_profile.cbegin(), lower);
    qsizetype last = std::distance(m_profile.cbegin(), upper); // exclusive
    if (first > 0)
        first--;
    if (last < m_profile.size())
        last++;
    const qsizetype visibleCount = last - first;
    if (visibleCount < 2) {
        fillGeometry->allocate(0);
        lineGeometry->allocate(0);
        node->fill()->markDirty(QSGNode::DirtyGeometry);
        node->line()->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

    // about one point per pixel is enough
    const double visiblePx = (m_profile[last - 1].x() - m_profile[first].x()) * sx;
    qsizetype step = visiblePx > 0 ? qsizetype(std::floor(visibleCount / visiblePx)) : 1;
    if (step < 1)
        step = 1;

    QList<QPointF> pts;
    pts.reserve(visibleCount / step + 2);
    for (qsizetype i = first; i < last; i += step)
        pts.append(QPointF(px(m_profile[i].x()), py(m_profile[i].y())));
    if ((last - 1 - first) % step != 0)
        pts.append(QPointF(px(m_profile[last - 1].x()), py(m_profile[last - 1].y())));

    const int n = static_cast<int>(pts.size());
    fillGeometry->allocate(n * 2);
    lineGeometry->allocate(n * 2);
    auto *fv = fillGeometry->vertexDataAsPoint2D();
    auto *lv = lineGeometry->vertexDataAsPoint2D();
    const float half = static_cast<float>(m_lineWidth / 2.0);
    const float bottom = static_cast<float>(h);
    for (int i = 0; i < n; ++i) {
        const float x = static_cast<float>(pts[i].x());
        const float y = static_cast<float>(pts[i].y());
        fv[i * 2].set(x, y);
        fv[i * 2 + 1].set(x, bottom);
        lv[i * 2].set(x, y - half);
        lv[i * 2 + 1].set(x, y + half);
    }
    node->fill()->markDirty(QSGNode::DirtyGeometry);
    node->line()->markDirty(QSGNode::DirtyGeometry);
    return node;
}
