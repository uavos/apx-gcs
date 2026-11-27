#include "NavaiZoneGrid.h"
#include "NavaiMapPlan.h"

#include <QPainter>
#include <QPen>
#include <QtMath>

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTilePixels = 256.0; // Qt Location world width at zoom 0
constexpr double kEarthRadiusM = 6371008.8;
// The scale drawn is the one whose tile is closest to this width on screen.
constexpr double kReadableTilePixels = 250.0;
// Tiles closer to each other than this on screen merge into a solid fill.
constexpr double kMinStridePixels = 12.0;

// Web Mercator pixel row of a latitude in a world of this width.
double worldY(double latitude, double world)
{
    const double s = std::sin(qBound(-89.9, latitude, 89.9) * kPi / 180.0);
    return (0.5 - std::log((1.0 + s) / (1.0 - s)) / (4.0 * kPi)) * world;
}

double latitudeAtWorldY(double y, double world)
{
    return 180.0 / kPi * std::atan(std::sinh(kPi - 2.0 * kPi * y / world));
}

} // namespace

NavaiZoneGrid::NavaiZoneGrid(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
    connect(this, &QQuickItem::widthChanged, this, [this] { update(); });
    connect(this, &QQuickItem::heightChanged, this, [this] { update(); });
}

QObject *NavaiZoneGrid::plan() const
{
    return _plan;
}

void NavaiZoneGrid::setPlan(QObject *plan)
{
    NavaiMapPlan *mapPlan = qobject_cast<NavaiMapPlan *>(plan);
    if (mapPlan == _plan)
        return;
    if (_plan)
        disconnect(_plan, nullptr, this, nullptr);
    _plan = mapPlan;
    if (_plan) {
        connect(_plan, &NavaiMapPlan::planChanged, this, [this] { update(); });
    }
    emit planChanged();
    update();
}

void NavaiZoneGrid::setCenter(const QGeoCoordinate &center)
{
    if (center == _center)
        return;
    _center = center;
    emit viewChanged();
    update();
}

void NavaiZoneGrid::setZoomLevel(double zoomLevel)
{
    if (qFuzzyCompare(zoomLevel, _zoomLevel))
        return;
    _zoomLevel = zoomLevel;
    emit viewChanged();
    update();
}

void NavaiZoneGrid::setBearing(double bearing)
{
    if (qFuzzyCompare(bearing, _bearing))
        return;
    _bearing = bearing;
    emit viewChanged();
    update();
}

void NavaiZoneGrid::setPointer(const QGeoCoordinate &pointer)
{
    if (pointer == _pointer)
        return;
    _pointer = pointer;
    emit pointerChanged();

    // the picture changes only when the pointer moves to another tile
    const int scale = shownScale();
    if (scale != _hoverScale || (scale >= 0 && tileUnderPointer(scale) != _hoverTile))
        update();
}

double NavaiZoneGrid::pixelsPerDegree() const
{
    return kTilePixels * std::pow(2.0, _zoomLevel) / 360.0;
}

int NavaiZoneGrid::shownScale() const
{
    if (!_plan || _plan->scales().isEmpty())
        return -1;
    const QVector<NavaiScalePlan> &scales = _plan->scales();
    const QList<QGeoCoordinate> zone = _plan->polygon().perimeter();
    if (zone.isEmpty())
        return -1;

    const double pixelsPerMeter = pixelsPerDegree()
                                  / (kEarthRadiusM * kPi / 180.0
                                     * qMax(1e-9, std::cos(zone.first().latitude() * kPi / 180.0)));
    int best = 0;
    double bestDistance = 0.0;
    for (int i = 0; i < scales.size(); ++i) {
        const double distance = std::abs(
            std::log(scales[i].footprintM * pixelsPerMeter / kReadableTilePixels));
        if (i == 0 || distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
    }
    return scales[best].strideM * pixelsPerMeter >= kMinStridePixels ? best : -1;
}

QPoint NavaiZoneGrid::tileUnderPointer(int scale) const
{
    const QVector<QPointF> local = NavaiMapGrid::toLocal(
        {_plan->polygon().perimeter().first(), _pointer});
    const double stride = _plan->scales()[scale].strideM;
    return QPoint(qRound(local[1].x() / stride), qRound(local[1].y() / stride));
}

void NavaiZoneGrid::paint(QPainter *painter)
{
    _hoverScale = -1;
    if (!_plan || !_center.isValid() || width() <= 0 || height() <= 0)
        return;
    const int index = shownScale();
    if (index < 0)
        return;
    const NavaiScalePlan &scale = _plan->scales()[index];
    const QGeoCoordinate anchor = _plan->polygon().perimeter().first();

    // Screen pixel = world pixel - world pixel of the centre + item centre,
    // rotated by the map bearing around the item centre.
    const double pixelsPerDegree = this->pixelsPerDegree();
    const double world = pixelsPerDegree * 360.0;
    const QPointF centerPixel((_center.longitude() + 180.0) * pixelsPerDegree,
                              worldY(_center.latitude(), world));

    // Only the tiles in view are drawn: everything within the half diagonal
    // of the item, whatever the bearing.
    const double radius = std::hypot(width(), height()) / 2.0;
    const double south = latitudeAtWorldY(centerPixel.y() + radius, world);
    const double north = latitudeAtWorldY(centerPixel.y() - radius, world);
    const double west = _center.longitude() - radius / pixelsPerDegree;
    const double east = _center.longitude() + radius / pixelsPerDegree;

    // the lattice is in metres east and north of the first vertex of the zone
    const double metersPerDegreeLat = kEarthRadiusM * kPi / 180.0;
    const double metersPerDegreeLon = metersPerDegreeLat
                                      * qMax(1e-9, std::cos(anchor.latitude() * kPi / 180.0));
    const double anchorX = (anchor.longitude() + 180.0) * pixelsPerDegree;
    const double stridePixels = scale.strideM / metersPerDegreeLon * pixelsPerDegree;

    const double half = scale.footprintM / 2.0;
    const double halfLat = half / metersPerDegreeLat;
    const int rowLast = qFloor(((south - anchor.latitude()) * metersPerDegreeLat - half)
                               / scale.strideM);
    const int rowFirst = qCeil(((north - anchor.latitude()) * metersPerDegreeLat + half)
                               / scale.strideM);
    const int columnFirst = qFloor(((west - anchor.longitude()) * metersPerDegreeLon - half)
                                   / scale.strideM);
    const int columnLast = qCeil(((east - anchor.longitude()) * metersPerDegreeLon + half)
                                 / scale.strideM);

    _hoverScale = index;
    _hoverTile = _pointer.isValid() ? tileUnderPointer(index) : QPoint();
    QRectF hoverRect;

    // The outlines of a row of tiles are its top and bottom lines and the
    // two sides of every tile; all of them go to the painter in one call.
    QVector<QLineF> lines;
    for (const NavaiGridSpan &span : scale.spans) {
        if (span.y > rowFirst || span.y < rowLast)
            continue;
        const int x0 = qMax(span.x0, columnFirst);
        const int x1 = qMin(span.x1, columnLast);
        if (x1 < x0)
            continue;

        // a tile is a square on the ground at its own latitude
        const double latitude = anchor.latitude() + span.y * scale.strideM / metersPerDegreeLat;
        const double top = worldY(latitude + halfLat, world);
        const double bottom = worldY(latitude - halfLat, world);
        const double halfWidth = halfLat / qMax(1e-9, std::cos(latitude * kPi / 180.0))
                                 * pixelsPerDegree;

        const double left = anchorX + x0 * stridePixels - halfWidth;
        const double right = anchorX + x1 * stridePixels + halfWidth;
        lines.append(QLineF(left, top, right, top));
        lines.append(QLineF(left, bottom, right, bottom));
        for (int x = x0; x <= x1; ++x) {
            const double middle = anchorX + x * stridePixels;
            lines.append(QLineF(middle - halfWidth, top, middle - halfWidth, bottom));
            lines.append(QLineF(middle + halfWidth, top, middle + halfWidth, bottom));
        }
        if (_pointer.isValid() && span.y == _hoverTile.y() && _hoverTile.x() >= span.x0
            && _hoverTile.x() <= span.x1) {
            const double middle = anchorX + _hoverTile.x() * stridePixels;
            hoverRect = QRectF(QPointF(middle - halfWidth, top),
                               QPointF(middle + halfWidth, bottom));
        }
    }
    if (lines.isEmpty())
        return;

    painter->save();
    painter->translate(QPointF(width() / 2.0, height() / 2.0));
    painter->rotate(-_bearing);
    painter->translate(-centerPixel);

    const QColor color(NavaiMapPlan::scaleColor(index));
    QColor line = color;
    line.setAlphaF(0.75);
    painter->setPen(QPen(line, 1.0));
    painter->drawLines(lines);

    // the tile under the pointer: what one frame covers
    if (hoverRect.isValid()) {
        QColor fill = color;
        fill.setAlphaF(0.3);
        painter->setPen(QPen(color, 2.0));
        painter->setBrush(fill);
        painter->drawRect(hoverRect);
    }

    painter->restore();
}
