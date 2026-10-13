#pragma once

#include <QGeoCoordinate>
#include <QQuickPaintedItem>

class NavaiOverlay;

// Paints the NAVAI tile grid and its scores over the map in one pass.
// The item covers the map; the view is described by the map's centre,
// zoom level and bearing, bound from QML.
class NavaiHeatmap : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *navai READ navai WRITE setNavai NOTIFY navaiChanged)
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY viewChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY viewChanged)
    Q_PROPERTY(double bearing READ bearing WRITE setBearing NOTIFY viewChanged)
    Q_PROPERTY(double fillOpacity READ fillOpacity WRITE setFillOpacity NOTIFY styleChanged)

public:
    explicit NavaiHeatmap(QQuickItem *parent = nullptr);

    QObject *navai() const;
    void setNavai(QObject *navai);

    QGeoCoordinate center() const { return _center; }
    void setCenter(const QGeoCoordinate &center);
    double zoomLevel() const { return _zoomLevel; }
    void setZoomLevel(double zoomLevel);
    double bearing() const { return _bearing; }
    void setBearing(double bearing);
    double fillOpacity() const { return _fillOpacity; }
    void setFillOpacity(double opacity);

    void paint(QPainter *painter) override;

signals:
    void navaiChanged();
    void viewChanged();
    void styleChanged();

private:
    // Web Mercator pixel of a coordinate at the current zoom, world origin.
    QPointF worldPixel(double latitude, double longitude) const;
    double latitudeAtWorldY(double y) const;
    double longitudeAtWorldX(double x) const;

    NavaiOverlay *_navai = nullptr;
    QGeoCoordinate _center;
    double _zoomLevel = 0.0;
    double _bearing = 0.0;
    double _fillOpacity = 0.45;
};
