#include "kmlgeopolygon.h"

#include <QPainter>
#include <QGeoRectangle>
#include <math.h>

KmlGeoPolygon::KmlGeoPolygon(QQuickItem *parent):
    QQuickPaintedItem(parent),
    m_map(nullptr)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(false);
    setMipmap(true);
}

void KmlGeoPolygon::registerQmlType()
{
    qmlRegisterType<KmlGeoPolygon>("KmlGeoPolygon", 1, 0, "KmlGeoPolygon");
}

void KmlGeoPolygon::clearPolygon()
{
    m_polygon.clear();
    m_holes.clear();
}

void KmlGeoPolygon::prepareForDrawing()
{
    QRegion clip(m_polygon);

    for(int i = 0; i < m_holes.size(); i++)
        clip -= QRegion(m_holes[i]);

    QRegion window(QRect(x(), y(), width(), height()));
    m_clip = clip;//.intersected(window);
}

void KmlGeoPolygon::setGeoPolygon(const QGeoPolygon &geoPolygon)
{
    m_geoPolygon = geoPolygon;
    recalcCoordinates();
    prepareForDrawing();
}

QGeoPolygon KmlGeoPolygon::getGeoPolygon() const
{
    return m_geoPolygon;
}

void KmlGeoPolygon::setMap(QObject *map)
{
    m_map = map;
    recalcCoordinates();
    prepareForDrawing();
}

QObject *KmlGeoPolygon::getMap() const
{
    return m_map;
}

QGeoShape KmlGeoPolygon::getArea() const
{
    return m_area;
}

void KmlGeoPolygon::setArea(const QGeoShape &area)
{
    m_area = area;
    if(m_map)
    {
        recalcCoordinates();
        prepareForDrawing();
        update();
    }
}

void KmlGeoPolygon::setColor(const QColor &color)
{
    if(m_color != color)
    {
        m_color = color;
        emit colorChanged();
        update();
    }
}

QColor KmlGeoPolygon::getColor() const
{
    return m_color;
}

void KmlGeoPolygon::paint(QPainter *painter)
{
    painter->setPen(m_color);

    painter->setClipRegion(m_clip);
    QPainterPath p;
    p.addPolygon(m_polygon);
    painter->fillPath(p, QBrush(m_color, Qt::SolidPattern));
}

QPoint KmlGeoPolygon::fromCoordinate(const QGeoCoordinate &c)
{
    QPointF point;
    QMetaObject::invokeMethod(m_map, "fromCoordinate",
                              Q_RETURN_ARG(QPointF, point),
                              Q_ARG(QGeoCoordinate, c),
                              Q_ARG(bool, false));
    return point.toPoint();
}

void KmlGeoPolygon::recalcCoordinates()
{
    clearPolygon();
    auto visibleRegion = m_map->property("visibleRegion").value<QGeoShape>().boundingGeoRectangle();
    QRect vr(fromCoordinate(visibleRegion.topLeft()),
             fromCoordinate(visibleRegion.bottomRight()));
    for(int i = 0; i < m_geoPolygon.size(); i++)
    {
        QPoint point = fromCoordinate(m_geoPolygon.coordinateAt(i));
        if(std::isnan(point.x()) || std::isnan(point.y()))
            continue;

        m_polygon.append(point);
    }
    for(int j = 0; j < m_geoPolygon.holesCount(); j++)
    {
        QPolygon polyHole;
        for(int k = 0; k < m_geoPolygon.hole(j).size(); k++)
        {
            QPoint point = fromCoordinate(m_geoPolygon.hole(j)[k].value<QGeoCoordinate>());
            if(std::isnan(point.x()) || std::isnan(point.y()))
                continue;
            polyHole.append(point);
        }
        m_holes.append(polyHole);
    }

    setX(vr.x());
    setY(vr.y());
    setWidth(vr.width());
    setHeight(vr.height());
}
