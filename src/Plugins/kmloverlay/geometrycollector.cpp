#include "geometrycollector.h"

#include <functional>
#include <QDomNodeList>
#include "ApxLog.h"

using namespace std::placeholders;

GeometryCollector::GeometryCollector()
{

}

void GeometryCollector::parse(const QByteArray &data)
{
    m_polygons.clear();

    QString errorMessage;
    int errorLine;
    if(m_dom.setContent(data, &errorMessage, &errorLine))
    {
        auto cb = std::bind(&GeometryCollector::placemarkCallback, this, _1);
        iterateOverChildrenElements(m_dom.documentElement(), "Placemark", cb);
    }
    else
        apxMsgW() << QString("%1 at line %2").arg(errorMessage, errorLine);
}

QList<QGeoPolygon> GeometryCollector::getPolygons()
{
    return m_polygons;
}

void GeometryCollector::iterateOverChildrenElements(const QDomElement &parent, const QString &tagname,
                                                    IterateCallback cb)
{
    auto children = parent.elementsByTagName(tagname);
    for(int i = 0; i < children.size(); i++)
    {
        auto c = children.at(i).toElement();
        if(!c.isNull())
            cb(c);
    }
}

void GeometryCollector::placemarkCallback(const QDomElement &el)
{
    auto styles = el.elementsByTagName("PolyStyle");
    if(!styles.isEmpty())
    {
        //style parser
    }
    auto cb = std::bind(&GeometryCollector::polygonCallback, this, _1);
    iterateOverChildrenElements(el, "Polygon", cb);
}

void GeometryCollector::polygonCallback(const QDomElement &el)
{
    auto cb = std::bind(&GeometryCollector::coordinatesCallback, this, _1);
    iterateOverChildrenElements(el, "coordinates", cb);
}

void GeometryCollector::coordinatesCallback(const QDomElement &el)
{
    QGeoPolygon polygon;
    QString coordinates = el.text().simplified();
    QStringList tuples = coordinates.split(" ", QString::SkipEmptyParts);
    for(auto &t: tuples)
    {
        QStringList coordinates = t.split(",", QString::SkipEmptyParts);
        bool ok1, ok2;
        double lon = t.section(",", 0, 0).toDouble(&ok1);
        double lat = t.section(",", 1, 1).toDouble(&ok2);
        if(ok1 && ok2)
        {
            polygon.addCoordinate(QGeoCoordinate(lat, lon));
        }
        else
            apxMsgW() << "Can't parse lat-lon from string " << t;
    }
    m_polygons.append(polygon);
}
