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

void TerrainProfileItem::setSegmentLength(double v)
{
    if (qFuzzyCompare(m_segmentLength, v))
        return;
    m_segmentLength = v;
    emit segmentLengthChanged();
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

double TerrainProfileItem::elevationAt(double missionDistance) const
{
    if (m_profile.size() < 2)
        return NAN;
    const double profileLength = m_profile.last().x();
    const double stretch = (m_segmentLength > 0 && profileLength > 0)
                               ? m_segmentLength / profileLength
                               : 1.0;
    const double d = (missionDistance - m_xOffset) / stretch;
    if (d < m_profile.first().x() || d > profileLength)
        return NAN;
    auto it = std::lower_bound(m_profile.cbegin(),
                               m_profile.cend(),
                               d,
                               [](const QPointF &p, double v) { return p.x() < v; });
    if (it == m_profile.cbegin())
        return it->y();
    const QPointF &b = *it;
    const QPointF &a = *(it - 1);
    const double span = b.x() - a.x();
    if (span <= 0)
        return b.y();
    return a.y() + (b.y() - a.y()) * (d - a.x()) / span;
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

    // a stale profile stretched to a new segment length is provisional: draw it dimmed
    const double profileLength = m_profile.size() >= 2 ? m_profile.last().x() : 0;
    const bool stale = m_segmentLength > 0 && profileLength > 0
                       && std::abs(m_segmentLength - profileLength) > 0.01 * profileLength + 1.0;
    if (stale != m_stale) {
        m_stale = stale;
        m_materialDirty = true;
    }
    if (m_materialDirty) {
        QColor fill = m_fillColor;
        QColor line = m_lineColor;
        if (m_stale) {
            fill.setAlphaF(fill.alphaF() * 0.4);
            line.setAlphaF(line.alphaF() * 0.4);
        }
        static_cast<QSGFlatColorMaterial *>(node->fill()->material())->setColor(fill);
        static_cast<QSGFlatColorMaterial *>(node->line()->material())->setColor(line);
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
    // stretch a stale profile to the current segment length
    const double stretch = (m_segmentLength > 0 && profileLength > 0)
                               ? m_segmentLength / profileLength
                               : 1.0;
    auto px = [&](double d) { return (d * stretch + m_xOffset - m_viewStart) * sx; };
    auto py = [&](double e) { return h - (e - m_minHeight) * sy; };

    // visible part of the profile (distances are monotonic), one extra point at each side
    const double dFrom = (m_viewStart - m_xOffset) / stretch;
    const double dTo = (m_viewStart + m_viewSpan - m_xOffset) / stretch;
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

    // Downsample to one point per pixel column keeping the HIGHEST elevation of the
    // column: a peak between two samples must never disappear from the chart.
    QList<QPointF> pts;
    pts.reserve(static_cast<qsizetype>(std::ceil(w)) + 2);
    double bucketX = std::floor(px(m_profile[first].x()));
    QPointF best(px(m_profile[first].x()), py(m_profile[first].y()));
    for (qsizetype i = first + 1; i < last; ++i) {
        const double x = px(m_profile[i].x());
        const double y = py(m_profile[i].y());
        const double bx = std::floor(x);
        if (bx != bucketX) {
            pts.append(best);
            bucketX = bx;
            best = QPointF(x, y);
            continue;
        }
        if (y < best.y()) // screen y grows downwards: smaller y is higher terrain
            best = QPointF(x, y);
    }
    pts.append(best);

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
