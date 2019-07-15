#ifndef GEOMETRYCOLLECTOR_H
#define GEOMETRYCOLLECTOR_H

#include <functional>
#include <QPolygonF>
#include <QColor>
#include <QDomDocument>

struct KmlPolygon
{
    QColor color;
    QPolygonF polygon;
};

class GeometryCollector
{
public:
    GeometryCollector();
    void parse(const QByteArray &data);
    QList<QPolygonF> getPolygons();

private:
    using IterateCallback = std::function<void(const QDomElement&)>;

    QList<QPolygonF> m_polygons;
    QDomDocument m_dom;

    void iterateOverChildrenElements(const QDomElement &parent, const QString &tagname, IterateCallback cb);

    void placemarkCallback(const QDomElement &el);
    void polygonCallback(const QDomElement &el);
    void coordinatesCallback(const QDomElement &el);
};

#endif // GEOMETRYCOLLECTOR_H
