#pragma once

#include <QGeoCoordinate>
#include <QPoint>
#include <QQuickPaintedItem>
#include <QVector>

class NavaiMapPlan;

// Paints the tiles of the planned map over the zone: the footprint of every
// tile in view and the one under the mouse pointer filled, to show what a
// single frame covers. One scale is drawn at a time, the one whose tiles
// have a readable size at the current zoom: zooming in goes from the highest
// altitude to the lowest. The view is described the same way as for
// NavaiHeatmap.
class NavaiZoneGrid : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *plan READ plan WRITE setPlan NOTIFY planChanged)
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY viewChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY viewChanged)
    Q_PROPERTY(double bearing READ bearing WRITE setBearing NOTIFY viewChanged)
    Q_PROPERTY(QGeoCoordinate pointer READ pointer WRITE setPointer NOTIFY pointerChanged)

public:
    explicit NavaiZoneGrid(QQuickItem *parent = nullptr);

    QObject *plan() const;
    void setPlan(QObject *plan);

    QGeoCoordinate center() const { return _center; }
    void setCenter(const QGeoCoordinate &center);
    double zoomLevel() const { return _zoomLevel; }
    void setZoomLevel(double zoomLevel);
    double bearing() const { return _bearing; }
    void setBearing(double bearing);
    QGeoCoordinate pointer() const { return _pointer; }
    void setPointer(const QGeoCoordinate &pointer);

    void paint(QPainter *painter) override;

signals:
    void planChanged();
    void viewChanged();
    void pointerChanged();

private:
    // Pixels of the map per degree of longitude at the current zoom.
    double pixelsPerDegree() const;
    // The scale to draw at the current zoom, -1 when its tiles are too small.
    int shownScale() const;
    // Column and row of the tile of this scale under the pointer.
    QPoint tileUnderPointer(int scale) const;

    NavaiMapPlan *_plan = nullptr;
    QGeoCoordinate _center;
    double _zoomLevel = 0.0;
    double _bearing = 0.0;
    QGeoCoordinate _pointer;
    int _hoverScale = -1;
    QPoint _hoverTile;
};
