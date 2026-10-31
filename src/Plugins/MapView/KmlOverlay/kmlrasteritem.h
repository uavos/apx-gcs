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

#include "kmlrenderer.h"

#include <QFutureWatcher>
#include <QGeoCoordinate>
#include <QGeoShape>
#include <QPointer>
#include <QQuickItem>

class KmlOverlay;

// Single textured item that shows all KML zones of the current viewport.
// The image is rendered in a worker thread for the viewport plus a margin,
// so panning only moves the texture and zooming scales it until a fresh
// image for the new zoom level arrives.
class KmlRasterItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *overlay READ overlay WRITE setOverlay NOTIFY overlayChanged)
    Q_PROPERTY(QGeoCoordinate origin READ origin NOTIFY frameChanged)
    Q_PROPERTY(double renderZoom READ renderZoom NOTIFY frameChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY frameChanged)

public:
    explicit KmlRasterItem(QQuickItem *parent = nullptr);

    static void registerQmlType();

    QObject *overlay() const;
    void setOverlay(QObject *overlay);

    QGeoCoordinate origin() const { return KmlMercator::toGeo(m_rect.topLeft()); }
    double renderZoom() const { return m_zoom; }
    bool ready() const { return m_ready; }

    Q_INVOKABLE void scheduleRender(const QGeoShape &visibleRegion,
                                    double zoomLevel,
                                    bool force = false);
    Q_INVOKABLE bool covers(const QGeoShape &visibleRegion) const;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private slots:
    void onDataChanged();
    void onRenderFinished();

private:
    QPointer<KmlOverlay> m_overlay;

    QFutureWatcher<KmlRenderResult> m_watcher;
    KmlRenderRequest m_pending;
    bool m_hasPending{false};
    quint64 m_serial{0};

    QImage m_image;
    bool m_imageDirty{false};
    QRectF m_rect;
    double m_zoom{0};
    bool m_ready{false};
    KmlDataSetPtr m_renderedData;

    QGeoShape m_lastRegion;
    double m_lastZoom{0};
    bool m_hasLast{false};

    void start(const KmlRenderRequest &request);
    void clearFrame();

signals:
    void overlayChanged();
    void frameChanged();
};
