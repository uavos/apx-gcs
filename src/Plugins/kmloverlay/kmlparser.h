#ifndef KMLPARSER_H
#define KMLPARSER_H

#include <functional>
#include <QPolygonF>
#include <QColor>
#include <QDomDocument>

struct KmlPolygon
{
    QColor color;
    QPolygonF data;
};

class KmlParser
{
public:
    KmlParser();
    void parse(const QByteArray &data);
    QList<KmlPolygon> getPolygons();

private:
    using IterateCallback = std::function<void(const QDomElement&)>;

    QList<KmlPolygon> m_polygons;
    QDomDocument m_dom;

    void iterateOverChildrenElements(const QDomElement &parent, const QString &tagname, IterateCallback cb);

    void placemarkCallback(const QDomElement &el);
    void polygonCallback(const QDomElement &el, const QColor &color);
    void coordinatesCallback(const QDomElement &el, KmlPolygon &polygon);
};

#endif // KMLPARSER_H
