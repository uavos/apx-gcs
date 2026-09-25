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
#include <QList>
#include <QPointF>
#include <QPointer>
#include <QQuickItem>

class MissionItem;

/**
 * Terrain profile of one mission item drawn directly with the Qt Quick scene graph.
 *
 * The item covers the chart plot area. Horizontal mapping is
 * (distance + xOffset - viewStart) / viewSpan * width, vertical mapping is
 * (elevation - minHeight) / (maxHeight - minHeight). Geometry is rebuilt on the
 * render thread only when something changes, rendering is done by the GPU,
 * so zooming and panning only cost a few property updates per frame.
 */
class TerrainProfileItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *missionItem READ missionItem WRITE setMissionItem NOTIFY missionItemChanged)
    Q_PROPERTY(
        QObject *elevationMap READ elevationMap WRITE setElevationMap NOTIFY elevationMapChanged)
    Q_PROPERTY(double xOffset READ xOffset WRITE setXOffset NOTIFY xOffsetChanged)
    Q_PROPERTY(
        double segmentLength READ segmentLength WRITE setSegmentLength NOTIFY segmentLengthChanged)
    Q_PROPERTY(double viewStart READ viewStart WRITE setViewStart NOTIFY viewStartChanged)
    Q_PROPERTY(double viewSpan READ viewSpan WRITE setViewSpan NOTIFY viewSpanChanged)
    Q_PROPERTY(double minHeight READ minHeight WRITE setMinHeight NOTIFY minHeightChanged)
    Q_PROPERTY(double maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(double lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(int pointCount READ pointCount NOTIFY pointCountChanged)

public:
    explicit TerrainProfileItem(QQuickItem *parent = nullptr);

    QObject *missionItem() const;
    void setMissionItem(QObject *v);
    // plugin instance: provides the lowest terrain across the corridor (thin line)
    QObject *elevationMap() const;
    void setElevationMap(QObject *v);

    double xOffset() const { return m_xOffset; }
    void setXOffset(double v);
    // current segment length [m]; while a fresh profile is being computed the
    // previous one is stretched to this length so the chart follows the waypoint
    double segmentLength() const { return m_segmentLength; }
    void setSegmentLength(double v);
    double viewStart() const { return m_viewStart; }
    void setViewStart(double v);
    double viewSpan() const { return m_viewSpan; }
    void setViewSpan(double v);
    double minHeight() const { return m_minHeight; }
    void setMinHeight(double v);
    double maxHeight() const { return m_maxHeight; }
    void setMaxHeight(double v);
    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &v);
    QColor lineColor() const { return m_lineColor; }
    void setLineColor(const QColor &v);
    double lineWidth() const { return m_lineWidth; }
    void setLineWidth(double v);
    int pointCount() const { return static_cast<int>(m_profile.size()); }

    // Terrain elevation [m] at the mission distance (interpolated between samples).
    // Returns NaN when the distance is outside this segment.
    Q_INVOKABLE double elevationAt(double missionDistance) const;

signals:
    void missionItemChanged();
    void elevationMapChanged();
    void xOffsetChanged();
    void segmentLengthChanged();
    void viewStartChanged();
    void viewSpanChanged();
    void minHeightChanged();
    void maxHeightChanged();
    void fillColorChanged();
    void lineColorChanged();
    void lineWidthChanged();
    void pointCountChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private slots:
    void reloadProfile();

private:
    void markGeometryDirty();

    QPointer<MissionItem> m_item;
    QList<QPointF>
        m_profile; // (distance from segment start [m], highest elevation across the corridor [m])
    QList<QPointF> m_profileCenter; // same distances, lowest elevation across the corridor (may be empty)
    QPointer<QObject> m_elevationMap;
    double m_xOffset{0};
    double m_segmentLength{0};
    double m_viewStart{0};
    double m_viewSpan{1000};
    double m_minHeight{0};
    double m_maxHeight{200};
    QColor m_fillColor{0, 255, 0, 64};
    QColor m_lineColor{0, 255, 0, 255};
    double m_lineWidth{1.5};

    bool m_geometryDirty{true};
    bool m_stale{false};       // drawn stretched while a fresh profile is computed
    bool m_placeholder{false}; // no profile: a thin bar marks the segment
    bool m_materialDirty{true};
};
