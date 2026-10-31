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
#include "kmlrasteritem.h"

#include "kmloverlay.h"

#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QtConcurrent>
#include <QtQml>

namespace {

constexpr double kMargin = 0.5;              // extra viewport fraction on each side
constexpr double kMaxTextureSize = 4096.0;   // device px, margin is reduced to fit
constexpr double kHardMaxTextureSize = 8192; // device px, the viewport is cropped to fit

} // namespace

KmlRasterItem::KmlRasterItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &KmlRasterItem::onRenderFinished);
}

void KmlRasterItem::registerQmlType()
{
    qmlRegisterType<KmlRasterItem>("KmlOverlay", 1, 0, "KmlRasterItem");
}

QObject *KmlRasterItem::overlay() const
{
    return m_overlay;
}

void KmlRasterItem::setOverlay(QObject *overlay)
{
    auto o = qobject_cast<KmlOverlay *>(overlay);
    if (m_overlay == o)
        return;

    if (m_overlay)
        disconnect(m_overlay, nullptr, this, nullptr);

    m_overlay = o;
    if (m_overlay)
        connect(m_overlay, &KmlOverlay::dataChanged, this, &KmlRasterItem::onDataChanged);

    emit overlayChanged();
    onDataChanged();
}

void KmlRasterItem::onDataChanged()
{
    if (m_hasLast)
        scheduleRender(m_lastRegion, m_lastZoom, true);
    else if (!m_overlay || !m_overlay->dataSet())
        clearFrame();
}

bool KmlRasterItem::covers(const QGeoShape &visibleRegion) const
{
    if (!m_ready || !visibleRegion.isValid())
        return false;
    return m_rect.contains(KmlMercator::fromGeoRect(visibleRegion.boundingGeoRectangle()));
}

void KmlRasterItem::scheduleRender(const QGeoShape &visibleRegion, double zoomLevel, bool force)
{
    if (!visibleRegion.isValid())
        return;

    m_lastRegion = visibleRegion;
    m_lastZoom = zoomLevel;
    m_hasLast = true;

    KmlDataSetPtr data = m_overlay ? m_overlay->dataSet() : KmlDataSetPtr();
    if (!data || data->zones.isEmpty()) {
        clearFrame();
        return;
    }

    const QRectF visible = KmlMercator::fromGeoRect(visibleRegion.boundingGeoRectangle());
    if (visible.isEmpty())
        return;

    const bool current = m_ready && data == m_renderedData
                         && qFuzzyCompare(m_zoom + 1.0, zoomLevel + 1.0)
                         && m_rect.contains(visible);
    if (current && !force)
        return;

    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const double scale = KmlRenderer::worldSize(zoomLevel) * dpr; // device px per unit

    // margin around the viewport, reduced when the texture would get too big
    double margin = kMargin;
    const double maxSide = qMax(visible.width(), visible.height()) * scale;
    if (maxSide > 0)
        margin = qBound(0.0, (kMaxTextureSize / maxSide - 1.0) / 2.0, kMargin);

    QRectF rect = visible.adjusted(-visible.width() * margin,
                                   -visible.height() * margin,
                                   visible.width() * margin,
                                   visible.height() * margin);
    rect &= QRectF(0, 0, 1, 1);

    const double cap = kHardMaxTextureSize / scale;
    if (rect.width() > cap) {
        const double cx = rect.center().x();
        rect.setLeft(cx - cap / 2);
        rect.setRight(cx + cap / 2);
    }
    if (rect.height() > cap) {
        const double cy = rect.center().y();
        rect.setTop(cy - cap / 2);
        rect.setBottom(cy + cap / 2);
    }

    KmlRenderRequest request;
    request.data = data;
    request.rect = rect;
    request.zoom = zoomLevel;
    request.dpr = dpr;
    request.serial = ++m_serial;

    if (m_watcher.isRunning()) {
        // keep only the latest request while the previous one is still rendering
        m_pending = request;
        m_hasPending = true;
        return;
    }

    start(request);
}

void KmlRasterItem::start(const KmlRenderRequest &request)
{
    m_hasPending = false;
    m_watcher.setFuture(
        QtConcurrent::run([request]() -> KmlRenderResult { return KmlRenderer::render(request); }));
}

void KmlRasterItem::onRenderFinished()
{
    const KmlRenderResult result = m_watcher.result();

    if (!result.image.isNull()) {
        m_image = result.image;
        m_imageDirty = true;
        m_rect = result.rect;
        m_zoom = result.zoom;
        m_renderedData = result.data;
        m_ready = true;
        setSize(QSizeF(m_image.width() / result.dpr, m_image.height() / result.dpr));
        emit frameChanged();
        update();
    }

    if (m_hasPending)
        start(m_pending);
}

void KmlRasterItem::clearFrame()
{
    if (!m_ready && m_image.isNull())
        return;

    m_ready = false;
    m_image = QImage();
    m_imageDirty = false;
    m_renderedData.reset();
    emit frameChanged();
    update();
}

QSGNode *KmlRasterItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto node = static_cast<QSGSimpleTextureNode *>(oldNode);

    if (!m_ready || m_image.isNull() || !window()) {
        delete node;
        return nullptr;
    }

    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setOwnsTexture(true);
        node->setFiltering(QSGTexture::Linear);
        m_imageDirty = true;
    }

    if (m_imageDirty) {
        QQuickWindow::CreateTextureOptions options(QQuickWindow::TextureHasAlphaChannel);
        options |= QQuickWindow::TextureHasMipmaps;
        QSGTexture *texture = window()->createTextureFromImage(m_image, options);
        if (texture) {
            texture->setFiltering(QSGTexture::Linear);
            texture->setMipmapFiltering(QSGTexture::Linear);
            node->setTexture(texture);
        }
        m_imageDirty = false;
    }

    node->setRect(boundingRect());
    return node;
}
