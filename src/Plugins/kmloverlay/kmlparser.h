#ifndef KMLPARSER_H
#define KMLPARSER_H

#include <functional>
#include <QColor>
#include <QDomDocument>
#include <QGeoPolygon>

struct KmlPolygon
{
    QColor color;
    QGeoPolygon data;
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
    void polygonOuterCallback(const QDomElement &el, KmlPolygon &polygon);
    void polygonInnerCallback(const QDomElement &el, KmlPolygon &polygon);
    void polygonCoordinatesCallback(const QDomElement &el, KmlPolygon &polygon);
    void polygonHolesCallback(const QDomElement &el, KmlPolygon &polygon);
    QList<QGeoCoordinate> parseCoordinates(const QString &text);
};

#endif // KMLPARSER_H
