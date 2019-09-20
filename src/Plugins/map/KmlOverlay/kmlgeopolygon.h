#ifndef KMLGEOPOLYGON_H
#define KMLGEOPOLYGON_H

#include <QGeoPolygon>
#include <QPolygon>
#include <QQuickPaintedItem>

class KmlGeoPolygon : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QGeoPolygon geoPolygon READ getGeoPolygon WRITE setGeoPolygon)
    Q_PROPERTY(QObject *map READ getMap WRITE setMap)
    Q_PROPERTY(QGeoShape area READ getArea WRITE setArea)
    Q_PROPERTY(QColor color READ getColor WRITE setColor NOTIFY colorChanged)
public:
    explicit KmlGeoPolygon(QQuickItem *parent = 0);

    static void registerQmlType();

    void setGeoPolygon(const QGeoPolygon &geoPolygon);
    QGeoPolygon getGeoPolygon() const;

    void setMap(QObject *map);
    QObject *getMap() const;

    QGeoShape getArea() const;
    void setArea(const QGeoShape &area);

    void setColor(const QColor &color);
    QColor getColor() const;

    void paint(QPainter *painter);

private:
    QObject *m_map;
    QGeoShape m_area;
    QGeoPolygon m_geoPolygon;
    QPolygon m_polygon;
    QVector<QPolygon> m_holes;
    QColor m_color;
    QRegion m_clip;

    QPoint fromCoordinate(const QGeoCoordinate &c);

    void clearPolygon();
    void prepareForDrawing();
    void recalcCoordinates();

signals:
    void colorChanged();
};

#endif // KMLGEOPOLYGON_H
