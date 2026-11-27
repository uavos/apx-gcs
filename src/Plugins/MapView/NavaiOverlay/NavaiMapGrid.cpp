#include "NavaiMapGrid.h"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kEarthRadiusM = 6371008.8; // the same as in navai's map/grid.py

double cosLatitude(const QGeoCoordinate &anchor)
{
    return qMax(1e-9, std::cos(qDegreesToRadians(anchor.latitude())));
}

} // namespace

double NavaiMapGrid::footprint(double altitudeM, double fovDeg)
{
    return 2.0 * altitudeM * std::tan(qDegreesToRadians(fovDeg) / 2.0);
}

QVector<QPointF> NavaiMapGrid::toLocal(const QList<QGeoCoordinate> &zone)
{
    QVector<QPointF> points;
    if (zone.isEmpty())
        return points;

    const QGeoCoordinate anchor = zone.first();
    const double cosLat = cosLatitude(anchor);
    for (const QGeoCoordinate &c : zone) {
        const double dLon = std::remainder(c.longitude() - anchor.longitude(), 360.0);
        points.append(QPointF(qDegreesToRadians(dLon) * kEarthRadiusM * cosLat,
                              qDegreesToRadians(c.latitude() - anchor.latitude())
                                  * kEarthRadiusM));
    }
    return points;
}

double NavaiMapGrid::areaM2(const QList<QGeoCoordinate> &zone)
{
    const QVector<QPointF> p = toLocal(zone);
    double sum = 0.0;
    for (int i = 0; i < p.size(); ++i) {
        const QPointF &a = p[i];
        const QPointF &b = p[(i + 1) % p.size()];
        sum += a.x() * b.y() - b.x() * a.y();
    }
    return std::abs(sum) / 2.0;
}

NavaiScalePlan NavaiMapGrid::plan(const QList<QGeoCoordinate> &zone,
                                  double altitudeM,
                                  double fovDeg,
                                  double overlap)
{
    NavaiScalePlan plan;
    plan.altitudeM = altitudeM;
    plan.footprintM = footprint(altitudeM, fovDeg);
    plan.overlap = overlap;
    plan.strideM = plan.footprintM * (1.0 - overlap);

    const double s = plan.strideM;
    if (zone.size() < 3 || !(s > 0.0) || !std::isfinite(s))
        return plan;

    const QVector<QPointF> p = toLocal(zone);
    double minY = p[0].y();
    double maxY = minY;
    for (const QPointF &point : p) {
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }

    struct Range
    {
        double a;
        double b;
    };
    QVector<Range> ranges;
    QVector<double> crossings;

    const int rowFirst = static_cast<int>(std::floor(maxY / s + 0.5));
    const int rowLast = static_cast<int>(std::ceil(minY / s - 0.5));
    for (int row = rowFirst; row >= rowLast; --row) {
        // the band of this row of cells
        const double ya = row * s - s / 2.0;
        const double yb = row * s + s / 2.0;
        ranges.clear();
        crossings.clear();

        for (int i = 0; i < p.size(); ++i) {
            const QPointF &a = p[i];
            const QPointF &b = p[(i + 1) % p.size()];
            const double y0 = qMin(a.y(), b.y());
            const double y1 = qMax(a.y(), b.y());

            // the part of the zone border inside the band
            if (y1 >= ya && y0 <= yb) {
                double xa = a.x();
                double xb = b.x();
                if (y1 - y0 > 1e-9) {
                    const double k = (b.x() - a.x()) / (b.y() - a.y());
                    xa = a.x() + (qMax(ya, y0) - a.y()) * k;
                    xb = a.x() + (qMin(yb, y1) - a.y()) * k;
                }
                ranges.append({qMin(xa, xb), qMax(xa, xb)});
            }

            // where the border crosses the lower line of the band
            if ((a.y() <= ya) != (b.y() <= ya))
                crossings.append(a.x() + (ya - a.y()) * (b.x() - a.x()) / (b.y() - a.y()));
        }

        // the inside of the zone on the lower line of the band
        std::sort(crossings.begin(), crossings.end());
        for (int i = 0; i + 1 < crossings.size(); i += 2)
            ranges.append({crossings[i], crossings[i + 1]});

        std::sort(ranges.begin(), ranges.end(), [](const Range &l, const Range &r) {
            return l.a < r.a;
        });

        // the cells of the row a range touches
        int firstOfRow = plan.spans.size();
        for (const Range &range : std::as_const(ranges)) {
            const int x0 = static_cast<int>(std::ceil(range.a / s - 0.5));
            const int x1 = static_cast<int>(std::floor(range.b / s + 0.5));
            if (x1 < x0)
                continue;
            if (plan.spans.size() > firstOfRow && x0 <= plan.spans.last().x1 + 1) {
                plan.spans.last().x1 = qMax(plan.spans.last().x1, x1);
                continue;
            }
            NavaiGridSpan span;
            span.y = row;
            span.x0 = x0;
            span.x1 = x1;
            plan.spans.append(span);
        }
    }

    for (const NavaiGridSpan &span : std::as_const(plan.spans))
        plan.tilesCount += span.x1 - span.x0 + 1;

    return plan;
}

void NavaiMapGrid::tileCenter(const QGeoCoordinate &anchor,
                              double strideM,
                              int x,
                              int y,
                              double *latitude,
                              double *longitude)
{
    *latitude = anchor.latitude() + qRadiansToDegrees(y * strideM / kEarthRadiusM);
    *longitude = anchor.longitude()
                 + qRadiansToDegrees(x * strideM / (kEarthRadiusM * cosLatitude(anchor)));
}
