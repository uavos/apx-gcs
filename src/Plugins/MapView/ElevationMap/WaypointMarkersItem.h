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

#include <QFont>
#include <QPointer>
#include <QQuickItem>
#include <QVector>

class Fact;
class Waypoint;

/**
 * Waypoint markers of the elevation chart (numbered boxes with vertical lines)
 * drawn with the scene graph in one item.
 *
 * The item covers the chart plot area and maps mission distance / height AMSL
 * to pixels itself, so zooming and panning only update transforms. Markers that
 * would overlap when zoomed out are skipped. A marker can be dragged vertically
 * to change the waypoint altitude, a click triggers the waypoint.
 */
class WaypointMarkersItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *group READ group WRITE setGroup NOTIFY groupChanged)
    Q_PROPERTY(double viewStart READ viewStart WRITE setViewStart NOTIFY viewStartChanged)
    Q_PROPERTY(double viewSpan READ viewSpan WRITE setViewSpan NOTIFY viewSpanChanged)
    Q_PROPERTY(double minHeight READ minHeight WRITE setMinHeight NOTIFY minHeightChanged)
    Q_PROPERTY(double maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged)
    Q_PROPERTY(bool hovered READ hovered NOTIFY hoveredChanged)

public:
    explicit WaypointMarkersItem(QQuickItem *parent = nullptr);
    ~WaypointMarkersItem() override;

    QObject *group() const;
    void setGroup(QObject *v);
    double viewStart() const { return m_viewStart; }
    void setViewStart(double v);
    double viewSpan() const { return m_viewSpan; }
    void setViewSpan(double v);
    double minHeight() const { return m_minHeight; }
    void setMinHeight(double v);
    double maxHeight() const { return m_maxHeight; }
    void setMaxHeight(double v);
    bool dragging() const { return m_drag >= 0; }
    bool hovered() const { return m_hover >= 0; }

    // re-reads waypoint data (positions, heights, alarm state)
    Q_INVOKABLE void refresh();
    // hover position (item coordinates) fed by the chart, so one HoverHandler
    // serves both the markers and the terrain readout
    Q_INVOKABLE void hoverAt(double x, double y);
    Q_INVOKABLE void hoverLeave();

signals:
    void groupChanged();
    void viewStartChanged();
    void viewSpanChanged();
    void minHeightChanged();
    void maxHeightChanged();
    void draggingChanged();
    void hoveredChanged();
    // a watched waypoint property changed, the owner should recompute its data and call refresh()
    void changed();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct Marker
    {
        QPointer<Waypoint> wp;
        int num{0};
        double distance{0}; // along the mission incl. runway part, m
        double height{0};   // AMSL, m
        bool alarm{false};
        // layout (pixels, GUI thread)
        double px{0};
        double py{0};
        double boxWidth{18};
        bool shown{false};
    };

    void layoutMarkers();
    int hitTest(const QPointF &pos) const;
    void setDrag(int index);
    void setHover(int index);

    QPointer<Fact> m_group;
    double m_viewStart{0};
    double m_viewSpan{1000};
    double m_minHeight{0};
    double m_maxHeight{200};

    QFont m_font;
    QVector<Marker> m_markers;
    QVector<double> m_textWidths;
    double m_textHeight{12};
    int m_hover{-1};
    int m_drag{-1};
    bool m_moved{false};

    bool m_markersDirty{true}; // markers list or text changed: nodes are rebuilt
    bool m_layoutDirty{true};  // positions changed: transforms and boxes are updated
};
