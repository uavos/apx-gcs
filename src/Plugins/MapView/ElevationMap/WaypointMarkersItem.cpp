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
#include "WaypointMarkersItem.h"

#include <Mission/MissionGroup.h>
#include <Mission/UnitMission.h>
#include <Mission/Waypoint.h>

#include <QFontMetricsF>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGOpacityNode>
#include <QSGTextNode>
#include <QSGVertexColorMaterial>
#include <QTextLayout>

#include <cmath>

namespace {

constexpr double BOX_HEIGHT = 18;
constexpr double MIN_BOX_WIDTH = 18;
constexpr double MIN_GAP = 4; // px between neighbour boxes, closer markers are skipped

struct Rgba
{
    uchar r, g, b, a;
};
const Rgba FILL{255, 255, 0, 255};
const Rgba FILL_ALARM{255, 222, 173, 255};
const Rgba BORDER{0, 0, 0, 64};
const Rgba BORDER_ALARM{255, 0, 0, 255};
const Rgba LINE{255, 255, 255, 127};

// root: child 0 = boxes and lines (vertex colored triangles), then one opacity node
// with a text node per marker
class MarkersNode : public QSGNode
{
public:
    MarkersNode()
    {
        shapes = new QSGGeometryNode;
        auto *g = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
        g->setDrawingMode(QSGGeometry::DrawTriangles);
        shapes->setGeometry(g);
        shapes->setFlag(QSGNode::OwnsGeometry);
        shapes->setMaterial(new QSGVertexColorMaterial);
        shapes->setFlag(QSGNode::OwnsMaterial);
        appendChildNode(shapes);
    }
    void clearTexts()
    {
        for (auto *n : texts) {
            removeChildNode(n);
            delete n;
        }
        texts.clear();
        textNodes.clear();
    }
    QSGGeometryNode *shapes;
    QVector<QSGOpacityNode *> texts;
    QVector<QSGTextNode *> textNodes;
};

void addRect(QSGGeometry::ColoredPoint2D *&v, float x, float y, float w, float h, Rgba c)
{
    v[0].set(x, y, c.r, c.g, c.b, c.a);
    v[1].set(x + w, y, c.r, c.g, c.b, c.a);
    v[2].set(x, y + h, c.r, c.g, c.b, c.a);
    v[3].set(x, y + h, c.r, c.g, c.b, c.a);
    v[4].set(x + w, y, c.r, c.g, c.b, c.a);
    v[5].set(x + w, y + h, c.r, c.g, c.b, c.a);
    v += 6;
}

} // namespace

WaypointMarkersItem::WaypointMarkersItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    m_font.setPixelSize(12);
    m_font.setBold(true);
    m_textHeight = QFontMetricsF(m_font).height();
}

WaypointMarkersItem::~WaypointMarkersItem() = default;

QObject *WaypointMarkersItem::group() const
{
    return m_group;
}

void WaypointMarkersItem::setGroup(QObject *v)
{
    auto f = qobject_cast<Fact *>(v);
    if (m_group == f)
        return;
    m_group = f;
    emit groupChanged();
    refresh();
}

void WaypointMarkersItem::setViewStart(double v)
{
    if (qFuzzyCompare(m_viewStart, v))
        return;
    m_viewStart = v;
    emit viewStartChanged();
    layoutMarkers();
}

void WaypointMarkersItem::setViewSpan(double v)
{
    if (qFuzzyCompare(m_viewSpan, v))
        return;
    m_viewSpan = v;
    emit viewSpanChanged();
    layoutMarkers();
}

void WaypointMarkersItem::setMinHeight(double v)
{
    if (qFuzzyCompare(m_minHeight, v))
        return;
    m_minHeight = v;
    emit minHeightChanged();
    layoutMarkers();
}

void WaypointMarkersItem::setMaxHeight(double v)
{
    if (qFuzzyCompare(m_maxHeight, v))
        return;
    m_maxHeight = v;
    emit maxHeightChanged();
    layoutMarkers();
}

void WaypointMarkersItem::setTopPadding(double v)
{
    if (qFuzzyCompare(m_topPadding, v))
        return;
    m_topPadding = v;
    emit topPaddingChanged();
    layoutMarkers();
}

void WaypointMarkersItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        layoutMarkers();
}

void WaypointMarkersItem::refresh()
{
    m_markers.clear();
    m_textWidths.clear();
    auto group = qobject_cast<MissionGroup *>(m_group.data());
    if (group && group->mission) {
        auto mission = group->mission;
        const auto p1 = mission->coordinate();
        const auto p2 = mission->startPoint();
        double acc = (p1.isValid() && p2.isValid()) ? p1.distanceTo(p2) : 0;
        const double startHmsl = std::round(mission->startElevation());
        const QFontMetricsF fm(m_font);
        for (int i = 0; i < group->size(); ++i) {
            auto wp = qobject_cast<Waypoint *>(group->child(i));
            if (!wp)
                continue;
            // watch what affects the marker; the owner recomputes and calls refresh()
            for (auto f : {wp->f_altitude, wp->f_amsl, wp->f_agl})
                connect(f,
                        &Fact::valueChanged,
                        this,
                        &WaypointMarkersItem::changed,
                        Qt::UniqueConnection);
            connect(wp,
                    &Waypoint::collisionChanged,
                    this,
                    &WaypointMarkersItem::changed,
                    Qt::UniqueConnection);
            connect(wp,
                    &MissionItem::elevationChanged,
                    this,
                    &WaypointMarkersItem::changed,
                    Qt::UniqueConnection);
            connect(wp,
                    &MissionItem::distanceChanged,
                    this,
                    &WaypointMarkersItem::changed,
                    Qt::UniqueConnection);

            acc += wp->distance();
            const double alt = wp->f_altitude->value().toDouble();
            const bool amsl = wp->f_amsl->value().toBool();
            Marker m;
            m.wp = wp;
            m.num = wp->num();
            m.distance = acc;
            m.height = amsl ? alt : alt + startHmsl;
            m.alarm = !std::isnan(wp->elevation())
                      && (wp->f_agl->value().toInt() < wp->unsafeAgl() || wp->collision());
            const double tw = fm.horizontalAdvance(QString::number(m.num + 1));
            m.boxWidth = qMax(MIN_BOX_WIDTH, tw + 4);
            m_markers.append(m);
            m_textWidths.append(tw);
        }
    }
    if (m_hover >= m_markers.size())
        setHover(-1);
    if (m_pressed >= m_markers.size())
        m_pressed = -1;
    m_markersDirty = true;
    layoutMarkers();
}

// pixel positions and overlap culling (GUI thread; used by painting and hit testing)
void WaypointMarkersItem::layoutMarkers()
{
    const double w = width();
    const double h = height() - m_topPadding; // plot height, the plot bottom is the item bottom
    const double hRange = m_maxHeight - m_minHeight;
    const bool valid = w > 0 && h > 0 && m_viewSpan > 0 && hRange > 0;
    const double sx = valid ? w / m_viewSpan : 0;
    const double sy = valid ? h / hRange : 0;
    double lastRight = -1e9;
    for (int i = 0; i < m_markers.size(); ++i) {
        Marker &m = m_markers[i];
        m.px = (m.distance - m_viewStart) * sx;
        m.py = height() - (m.height - m_minHeight) * sy;
        const double left = m.px - m.boxWidth / 2;
        const bool inView = valid && m.px + m.boxWidth / 2 >= 0 && left <= w;
        const bool fits = left >= lastRight + MIN_GAP;
        m.shown = inView && (fits || i == m_hover || i == m_pressed);
        if (m.shown)
            lastRight = m.px + m.boxWidth / 2;
    }
    m_layoutDirty = true;
    update();
}

QSGNode *WaypointMarkersItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *node = static_cast<MarkersNode *>(oldNode);
    if (!node) {
        node = new MarkersNode;
        m_markersDirty = true;
    }

    if (m_markersDirty) {
        m_markersDirty = false;
        m_layoutDirty = true;
        node->clearTexts();
        for (int i = 0; i < m_markers.size(); ++i) {
            auto *opacity = new QSGOpacityNode;
            auto *text = window()->createTextNode();
            QTextLayout layout(QString::number(m_markers[i].num + 1), m_font);
            layout.beginLayout();
            layout.createLine();
            layout.endLayout();
            text->addTextLayout(QPointF(0, 0), &layout);
            opacity->appendChildNode(text);
            node->appendChildNode(opacity);
            node->texts.append(opacity);
            node->textNodes.append(text);
        }
    }

    if (!m_layoutDirty)
        return node;
    m_layoutDirty = false;

    const double h = height();
    int shown = 0;
    for (const auto &m : m_markers)
        if (m.shown)
            shown++;

    // boxes: border + fill, vertical lines: 1 px quads
    QSGGeometry *g = node->shapes->geometry();
    g->allocate(shown * 3 * 6);
    auto *v = g->vertexDataAsColoredPoint2D();
    for (int i = 0; i < m_markers.size(); ++i) {
        const Marker &m = m_markers[i];
        auto *opacity = node->texts.value(i);
        if (!opacity)
            continue;
        opacity->setOpacity(m.shown ? 1.0 : 0.0);
        if (!m.shown)
            continue;
        const float bw = float(m.boxWidth);
        const float bh = float(BOX_HEIGHT);
        const float x = float(m.px - bw / 2);
        const float y = float(m.py - bh / 2);
        addRect(v, float(m.px - 0.5), float(m.py), 1, float(qMax(0.0, h - m.py)), LINE);
        addRect(v, x, y, bw, bh, m.alarm ? BORDER_ALARM : BORDER);
        addRect(v, x + 1, y + 1, bw - 2, bh - 2, m.alarm ? FILL_ALARM : FILL);

        auto *text = node->textNodes[i];
        text->setColor(m.alarm ? QColor(255, 0, 0) : QColor(0, 0, 0));
        QMatrix4x4 t;
        t.translate(float(m.px - m_textWidths[i] / 2), float(m.py - m_textHeight / 2));
        text->setMatrix(t);
        text->markDirty(QSGNode::DirtyMatrix);
    }
    node->shapes->markDirty(QSGNode::DirtyGeometry);

    // hovered/dragged marker on top
    const int top = m_pressed >= 0 ? m_pressed : m_hover;
    if (top >= 0 && top < node->texts.size()) {
        auto *opacity = node->texts[top];
        node->removeChildNode(opacity);
        node->appendChildNode(opacity);
    }
    return node;
}

// The number box with a margin around it, plus the vertical line under it:
// small boxes are hard to hit exactly, the line gives a tall target.
int WaypointMarkersItem::hitTest(const QPointF &pos) const
{
    constexpr double BOX_MARGIN = 6;
    constexpr double LINE_HALF_WIDTH = 5;
    // boxes first: they are on top of the lines
    for (int i = m_markers.size() - 1; i >= 0; --i) {
        const Marker &m = m_markers[i];
        if (!m.shown)
            continue;
        if (std::abs(pos.x() - m.px) <= m.boxWidth / 2 + BOX_MARGIN
            && std::abs(pos.y() - m.py) <= BOX_HEIGHT / 2 + BOX_MARGIN)
            return i;
    }
    for (int i = m_markers.size() - 1; i >= 0; --i) {
        const Marker &m = m_markers[i];
        if (!m.shown)
            continue;
        if (std::abs(pos.x() - m.px) <= LINE_HALF_WIDTH && pos.y() >= m.py && pos.y() <= height())
            return i;
    }
    return -1;
}

void WaypointMarkersItem::setHover(int index)
{
    if (index == m_hover)
        return;
    const bool was = m_hover >= 0;
    m_hover = index;
    if (was != (m_hover >= 0))
        emit hoveredChanged();
    layoutMarkers();
}

void WaypointMarkersItem::hoverAt(double x, double y)
{
    setHover(hitTest(QPointF(x, y)));
}

void WaypointMarkersItem::hoverLeave()
{
    setHover(-1);
}

void WaypointMarkersItem::mousePressEvent(QMouseEvent *event)
{
    const int idx = hitTest(event->position());
    if (idx < 0) {
        event->ignore(); // let the chart pan
        return;
    }
    m_pressed = idx;
    event->accept();
}

void WaypointMarkersItem::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

// a click on a marker: the map centers on the waypoint and its menu opens
void WaypointMarkersItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressed < 0)
        return;
    auto wp = m_pressed < m_markers.size() ? m_markers[m_pressed].wp : nullptr;
    const bool onMarker = hitTest(event->position()) == m_pressed;
    m_pressed = -1;
    event->accept();
    if (onMarker && wp)
        wp->trigger();
}
