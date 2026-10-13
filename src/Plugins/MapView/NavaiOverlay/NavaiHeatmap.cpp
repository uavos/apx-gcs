#include "NavaiHeatmap.h"
#include "NavaiOverlay.h"

#include <QPainter>
#include <QPen>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kTilePixels = 256.0;  // Qt Location world width at zoom 0
constexpr double kMetersPerDegree = 111320.0;
// Below this size on screen a tile is a dot: no outline, scored tiles only.
constexpr double kMinOutlinedPixels = 3.0;

QColor scoreColor(float score)
{
    const double normalized = qBound(0.0, static_cast<double>(score), 1.0);
    return QColor::fromHsvF(normalized * 0.33, 0.85, 0.95);
}
} // namespace

NavaiHeatmap::NavaiHeatmap(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
    // QQuickPaintedItem repaints only through its own update(); the lambdas
    // make sure the whole item is marked dirty.
    connect(this, &QQuickItem::widthChanged, this, [this] { update(); });
    connect(this, &QQuickItem::heightChanged, this, [this] { update(); });
}

QObject *NavaiHeatmap::navai() const
{
    return _navai;
}

void NavaiHeatmap::setNavai(QObject *navai)
{
    NavaiOverlay *overlay = qobject_cast<NavaiOverlay *>(navai);
    if (overlay == _navai)
        return;
    if (_navai)
        disconnect(_navai, nullptr, this, nullptr);
    _navai = overlay;
    if (_navai)
        connect(_navai, &NavaiOverlay::heatmapChanged, this, [this] { update(); });
    emit navaiChanged();
    update();
}

void NavaiHeatmap::setCenter(const QGeoCoordinate &center)
{
    if (center == _center)
        return;
    _center = center;
    emit viewChanged();
    update();
}

void NavaiHeatmap::setZoomLevel(double zoomLevel)
{
    if (qFuzzyCompare(zoomLevel, _zoomLevel))
        return;
    _zoomLevel = zoomLevel;
    emit viewChanged();
    update();
}

void NavaiHeatmap::setBearing(double bearing)
{
    if (qFuzzyCompare(bearing, _bearing))
        return;
    _bearing = bearing;
    emit viewChanged();
    update();
}

void NavaiHeatmap::setFillOpacity(double opacity)
{
    if (qFuzzyCompare(opacity, _fillOpacity))
        return;
    _fillOpacity = opacity;
    emit styleChanged();
    update();
}

QPointF NavaiHeatmap::worldPixel(double latitude, double longitude) const
{
    const double world = kTilePixels * std::pow(2.0, _zoomLevel);
    const double x = (longitude + 180.0) / 360.0 * world;
    const double s = std::sin(qBound(-89.9, latitude, 89.9) * kPi / 180.0);
    const double y = (0.5 - std::log((1.0 + s) / (1.0 - s)) / (4.0 * kPi)) * world;
    return {x, y};
}

double NavaiHeatmap::latitudeAtWorldY(double y) const
{
    const double world = kTilePixels * std::pow(2.0, _zoomLevel);
    const double n = kPi - 2.0 * kPi * y / world;
    return 180.0 / kPi * std::atan(std::sinh(n));
}

double NavaiHeatmap::longitudeAtWorldX(double x) const
{
    const double world = kTilePixels * std::pow(2.0, _zoomLevel);
    return x / world * 360.0 - 180.0;
}

void NavaiHeatmap::paint(QPainter *painter)
{
    if (!_navai || !_center.isValid() || width() <= 0 || height() <= 0)
        return;
    const NavaiTileMap &tiles = _navai->tileMap();
    if (tiles.count() == 0)
        return;

    // Screen pixel = world pixel - world pixel of the centre + item centre,
    // rotated by the map bearing around the item centre.
    const QPointF centerPixel = worldPixel(_center.latitude(), _center.longitude());
    const QPointF screenCenter(width() / 2.0, height() / 2.0);

    // Everything within the half diagonal of the view is drawn, whatever the bearing.
    const double radius = std::hypot(width(), height()) / 2.0;
    const double south = latitudeAtWorldY(centerPixel.y() + radius);
    const double north = latitudeAtWorldY(centerPixel.y() - radius);
    const double west = longitudeAtWorldX(centerPixel.x() - radius);
    const double east = longitudeAtWorldX(centerPixel.x() + radius);
    const QVector<int> visible = tiles.inside(south, north, west, east);
    if (visible.isEmpty())
        return;

    painter->save();
    painter->translate(screenCenter);
    painter->rotate(-_bearing);
    painter->translate(-centerPixel);

    const QPen outline(QColor(153, 153, 153, 38), 0.5);

    for (int index : visible) {
        const NavaiTile &tile = tiles.tile(index);
        const double halfLat = tile.sizeM / kMetersPerDegree / 2.0;
        const double halfLon = halfLat / qMax(0.1, std::cos(tile.latitude * kPi / 180.0));
        const QPointF topLeft = worldPixel(tile.latitude + halfLat, tile.longitude - halfLon);
        const QPointF bottomRight = worldPixel(tile.latitude - halfLat, tile.longitude + halfLon);
        QRectF rect(topLeft, bottomRight);

        const bool scored = tile.score >= 0.0f;
        if (rect.width() < kMinOutlinedPixels) {
            if (!scored)
                continue;
            // Keep a scored tile visible as at least one pixel.
            rect.setSize(QSizeF(qMax(1.0, rect.width()), qMax(1.0, rect.height())));
            painter->fillRect(rect, scoreColor(tile.score));
            continue;
        }

        if (!scored) {
            painter->setPen(outline);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect);
            continue;
        }

        const QColor color = scoreColor(tile.score);
        QColor fill = color;
        fill.setAlphaF(_fillOpacity);
        painter->setPen(QPen(color, 1.0));
        painter->setBrush(fill);
        painter->drawRect(rect);
    }

    painter->restore();
}
