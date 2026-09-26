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

#include "ElevationMap.h"

#include <Mission/MissionGroup.h>
#include <Mission/MissionItem.h>
#include <Mission/UnitMission.h>

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>

#include <algorithm>
#include <cmath>

namespace {

// root node: child 0 = fill (triangle strip), child 1 = border band (triangle strip),
// child 2 = lowest terrain across the corridor (thin band)
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
    QSGGeometryNode *minLine() { return nodes[2]; }

private:
    QSGGeometryNode *nodes[3]{nullptr, nullptr, nullptr};
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

QObject *TerrainProfileItem::elevationMap() const
{
    return m_elevationMap;
}

void TerrainProfileItem::setElevationMap(QObject *v)
{
    if (m_elevationMap == v)
        return;
    m_elevationMap = v;
    emit elevationMapChanged();
    reloadProfile();
}

void TerrainProfileItem::reloadProfile()
{
    const auto count = m_profile.size();
    m_profile = m_item ? m_item->terrainProfile() : QList<QPointF>();
    m_profileMin.clear();
    auto em = qobject_cast<ElevationMap *>(m_elevationMap.data());
    if (em && m_item && m_profile.size() >= 2) {
        // the profile was requested for the item path, the first item starts at the runway
        auto path = m_item->geoPath();
        auto mission = m_item->group ? m_item->group->mission : nullptr;
        if (m_item->num() == 0 && mission && mission->coordinate().isValid())
            path.insertCoordinate(0, mission->coordinate());
        auto min = em->minProfile(path);
        if (min.size() == m_profile.size())
            m_profileMin = min;
    }
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

    if ((m_profile.size() < 2) != m_placeholder) {
        m_placeholder = m_profile.size() < 2;
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
        if (m_placeholder)
            line = QColor(255, 255, 255, 96);
        static_cast<QSGFlatColorMaterial *>(node->fill()->material())->setColor(fill);
        static_cast<QSGFlatColorMaterial *>(node->line()->material())->setColor(line);
        QColor minColor(255,
                        176,
                        0,
                        m_stale ? 90 : 220); // amber, distinct from terrain and flight path
        static_cast<QSGFlatColorMaterial *>(node->minLine()->material())->setColor(minColor);
        node->minLine()->markDirty(QSGNode::DirtyMaterial);
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
    const bool noProfile = m_profile.size() < 2;
    if (noProfile && w > 0 && h > 0 && m_viewSpan > 0 && m_segmentLength > 0) {
        // no profile (not computed yet or no elevation data): thin bar at the bottom
        fillGeometry->allocate(0);
        const double sx = w / m_viewSpan;
        const float x0 = float((m_xOffset - m_viewStart) * sx);
        const float x1 = float((m_xOffset + m_segmentLength - m_viewStart) * sx);
        lineGeometry->allocate(4);
        auto *lv = lineGeometry->vertexDataAsPoint2D();
        lv[0].set(x0, float(h - 2));
        lv[1].set(x0, float(h));
        lv[2].set(x1, float(h - 2));
        lv[3].set(x1, float(h));
        node->minLine()->geometry()->allocate(0);
        node->fill()->markDirty(QSGNode::DirtyGeometry);
        node->line()->markDirty(QSGNode::DirtyGeometry);
        node->minLine()->markDirty(QSGNode::DirtyGeometry);
        return node;
    }
    if (noProfile || w <= 0 || h <= 0 || m_viewSpan <= 0 || hRange <= 0) {
        fillGeometry->allocate(0);
        lineGeometry->allocate(0);
        node->minLine()->geometry()->allocate(0);
        node->fill()->markDirty(QSGNode::DirtyGeometry);
        node->line()->markDirty(QSGNode::DirtyGeometry);
        node->minLine()->markDirty(QSGNode::DirtyGeometry);
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
        node->minLine()->geometry()->allocate(0);
        node->fill()->markDirty(QSGNode::DirtyGeometry);
        node->line()->markDirty(QSGNode::DirtyGeometry);
        node->minLine()->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

    // Downsample to one point per pixel column keeping the HIGHEST elevation of the
    // column: a peak between two samples must never disappear from the chart.
    // Both segment ends are always emitted and a segment narrower than a pixel is
    // widened to one pixel, so neighbouring segments never leave a gap.
    QList<QPointF> pts;
    pts.reserve(static_cast<qsizetype>(std::ceil(w)) + 3);
    const QPointF firstPt(px(m_profile[first].x()), py(m_profile[first].y()));
    QPointF lastPt(px(m_profile[last - 1].x()), py(m_profile[last - 1].y()));
    pts.append(firstPt);
    bool hasBest = false;
    double bucketX = std::floor(firstPt.x());
    QPointF best;
    for (qsizetype i = first + 1; i < last - 1; ++i) {
        const double x = px(m_profile[i].x());
        const double y = py(m_profile[i].y());
        const double bx = std::floor(x);
        if (bx != bucketX) {
            if (hasBest)
                pts.append(best);
            bucketX = bx;
            best = QPointF(x, y);
            hasBest = true;
            continue;
        }
        if (!hasBest || y < best.y()) { // screen y grows downwards: smaller y is higher terrain
            best = QPointF(x, y);
            hasBest = true;
        }
    }
    if (hasBest)
        pts.append(best);
    if (lastPt.x() < firstPt.x() + 1.0)
        lastPt.setX(firstPt.x() + 1.0);
    pts.append(lastPt);

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

    // lowest terrain across the corridor: thin band, one point per column (lowest)
    QSGGeometry *minGeometry = node->minLine()->geometry();
    if (m_profileMin.size() == m_profile.size()) {
        QList<QPointF> mpts;
        mpts.reserve(pts.size() + 2);
        double mBucket = std::floor(px(m_profileMin[first].x()));
        QPointF low(px(m_profileMin[first].x()), py(m_profileMin[first].y()));
        for (qsizetype i = first + 1; i < last; ++i) {
            const double x = px(m_profileMin[i].x());
            const double y = py(m_profileMin[i].y());
            const double bx = std::floor(x);
            if (bx != mBucket) {
                mpts.append(low);
                mBucket = bx;
                low = QPointF(x, y);
                continue;
            }
            if (y > low.y())
                low = QPointF(x, y);
        }
        mpts.append(low);
        const int mn = static_cast<int>(mpts.size());
        minGeometry->allocate(mn * 2);
        auto *mv = minGeometry->vertexDataAsPoint2D();
        for (int i = 0; i < mn; ++i) {
            const float x = static_cast<float>(mpts[i].x());
            const float y = static_cast<float>(mpts[i].y());
            mv[i * 2].set(x, y - 0.75f);
            mv[i * 2 + 1].set(x, y + 0.75f);
        }
    } else {
        minGeometry->allocate(0);
    }
    node->minLine()->markDirty(QSGNode::DirtyGeometry);
    return node;
}
