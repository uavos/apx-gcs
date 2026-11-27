#pragma once

#include <QGeoCoordinate>
#include <QList>
#include <QPointF>
#include <QVector>

// A run of tiles in one row of the lattice.
struct NavaiGridSpan
{
    int y = 0;  // lattice row, north is positive
    int x0 = 0; // first and last lattice column, east is positive
    int x1 = 0;
};

// The tiles of one flight altitude over a zone.
struct NavaiScalePlan
{
    double altitudeM = 0.0;
    double footprintM = 0.0; // side of a tile on the ground
    double overlap = 0.0;
    double strideM = 0.0; // distance between tile centres
    QVector<NavaiGridSpan> spans; // north to south, west to east
    qint64 tilesCount = 0;
};

// The tile lattice of a NAVAI map, the maths of navai's map/grid.py with a
// zone in place of the route. The lattice is anchored at the first vertex of
// the zone; a tile is taken when its cell, one stride wide around the tile
// centre, touches the zone, so every point of the zone is at most half a
// stride away from a tile centre.
namespace NavaiMapGrid {

// Side of the ground square a nadir camera sees from this height.
double footprint(double altitudeM, double fovDeg);

// The zone in metres east and north of its first vertex.
QVector<QPointF> toLocal(const QList<QGeoCoordinate> &zone);

double areaM2(const QList<QGeoCoordinate> &zone);

NavaiScalePlan plan(const QList<QGeoCoordinate> &zone,
                    double altitudeM,
                    double fovDeg,
                    double overlap);

// Centre of the tile in column x and row y of the lattice.
void tileCenter(const QGeoCoordinate &anchor,
                double strideM,
                int x,
                int y,
                double *latitude,
                double *longitude);

} // namespace NavaiMapGrid
