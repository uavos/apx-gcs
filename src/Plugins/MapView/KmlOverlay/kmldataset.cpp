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
#include "kmldataset.h"

#include "kmlparser.h"

#include <cmath>
#include <QPolygonF>
#include <QRegularExpression>

namespace KmlMercator {

QPointF fromGeo(const QGeoCoordinate &c)
{
    const double lat = qBound(-85.05112878, c.latitude(), 85.05112878);
    const double x = (c.longitude() + 180.0) / 360.0;
    const double s = std::sin(qDegreesToRadians(lat));
    const double y = 0.5 - std::log((1.0 + s) / (1.0 - s)) / (4.0 * M_PI);
    return QPointF(x, y);
}

QGeoCoordinate toGeo(const QPointF &p)
{
    const double lon = p.x() * 360.0 - 180.0;
    const double lat = qRadiansToDegrees(std::atan(std::sinh(M_PI * (1.0 - 2.0 * p.y()))));
    return QGeoCoordinate(lat, lon);
}

QRectF fromGeoRect(const QGeoRectangle &r)
{
    return QRectF(fromGeo(r.topLeft()), fromGeo(r.bottomRight())).normalized();
}

} // namespace KmlMercator

namespace {

QVector<QPointF> toRing(const QList<QGeoCoordinate> &coordinates)
{
    QVector<QPointF> ring;
    ring.reserve(coordinates.size());
    for (const auto &c : coordinates) {
        if (!c.isValid())
            continue;
        ring.append(KmlMercator::fromGeo(c));
    }
    // drop the explicit closing point, the path is closed when drawn
    while (ring.size() > 1 && ring.first() == ring.last())
        ring.removeLast();
    return ring;
}

QPointF centroid(const QVector<QPointF> &ring, const QRectF &bbox)
{
    double area = 0;
    double cx = 0;
    double cy = 0;
    const QPointF o = bbox.topLeft(); // keep the sums small for precision
    for (int i = 0, n = ring.size(); i < n; ++i) {
        const QPointF a = ring.at(i) - o;
        const QPointF b = ring.at((i + 1) % n) - o;
        const double f = a.x() * b.y() - b.x() * a.y();
        area += f;
        cx += (a.x() + b.x()) * f;
        cy += (a.y() + b.y()) * f;
    }
    if (qFuzzyIsNull(area * 1e12))
        return bbox.center();

    const QPointF c = QPointF(cx, cy) / (3.0 * area) + o;
    return bbox.contains(c) ? c : bbox.center();
}

// "UMP 175" -> "P175", anything else is kept as written
QString compactName(QString name)
{
    name = name.simplified();
    static const QRegularExpression re(QStringLiteral(R"(^([A-Z]{2})([A-Z])\s*[-_ ]?\s*(\d+)$)"),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(name);
    if (m.hasMatch())
        return (m.captured(2) + m.captured(3)).toUpper();
    if (name.size() > 24)
        name = name.left(23) + QChar(0x2026);
    return name;
}

// "1 050 (3 500)" -> "(3500)", "GND" -> "GND"
QString compactLimit(QString text)
{
    text = text.simplified();
    static const QRegularExpression paren(QStringLiteral(R"(\(([^)]+)\))"));
    const auto m = paren.match(text);
    if (m.hasMatch())
        text = QStringLiteral("(%1)").arg(m.captured(1));
    text.remove(' ');
    if (text.size() > 14)
        text = text.left(14);
    return text;
}

} // namespace

KmlDataSetPtr KmlDataSet::fromPolygons(const QList<KmlPolygon> &polygons)
{
    auto ds = QSharedPointer<KmlDataSet>::create();
    ds->zones.reserve(polygons.size());

    for (const auto &p : polygons) {
        KmlZone zone;
        zone.outer = toRing(p.data.perimeter());
        if (zone.outer.size() < 3)
            continue;

        for (qsizetype i = 0; i < p.data.holesCount(); ++i) {
            auto hole = toRing(p.data.holePath(i));
            if (hole.size() >= 3)
                zone.holes.append(hole);
        }

        zone.bbox = QPolygonF(zone.outer).boundingRect();
        zone.labelPos = centroid(zone.outer, zone.bbox);
        zone.color = p.color.isValid() ? p.color : QColor(Qt::red);
        zone.name = compactName(p.name);
        zone.upper = compactLimit(p.upper);
        zone.lower = compactLimit(p.lower);

        ds->bbox |= zone.bbox;
        ds->zones.append(zone);
    }

    return ds;
}
