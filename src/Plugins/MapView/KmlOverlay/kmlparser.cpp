/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "kmlparser.h"

#include <App/AppLog.h>
#include <QDomNodeList>

using namespace std::placeholders;

KmlParser::KmlParser() {}

void KmlParser::parse(const QByteArray &data)
{
    m_polygonId = 0;
    m_polygons.clear();

    QString errorMessage;
    int errorLine;
    if (m_dom.setContent(data, &errorMessage, &errorLine)) {
        auto cb = std::bind(&KmlParser::placemarkCallback, this, _1);
        iterateOverChildrenElements(m_dom.documentElement(), "Placemark", cb);
    } else
        apxMsgW() << QString("%1 at line %2").arg(errorMessage, errorLine);
}

QList<KmlPolygon> KmlParser::getPolygons()
{
    return m_polygons;
}

void KmlParser::iterateOverChildrenElements(const QDomElement &parent,
                                            const QString &tagname,
                                            const IterateCallback &cb)
{
    auto children = parent.elementsByTagName(tagname);
    for (int i = 0; i < children.size(); i++) {
        auto c = children.at(i).toElement();
        if (!c.isNull())
            cb(c);
    }
}

void KmlParser::placemarkCallback(const QDomElement &el)
{
    //polygon style parser
    QColor polygonColor("red");
    auto styles = el.elementsByTagName("PolyStyle");
    if (!styles.isEmpty()) {
        auto color = styles.at(0).toElement().elementsByTagName("color");
        if (!color.isEmpty())
            polygonColor.fromString("#" + color.at(0).toElement().text());
        if (!polygonColor.isValid())
            qDebug() << "not valid";
    }
    auto cb = std::bind(&KmlParser::polygonCallback, this, _1, polygonColor);
    iterateOverChildrenElements(el, "Polygon", cb);
}

void KmlParser::polygonCallback(const QDomElement &el, const QColor &color)
{
    KmlPolygon polygon;
    polygon.color = color;
    polygon.id = m_polygonId++;
    auto cbouter = std::bind(&KmlParser::polygonOuterCallback, this, _1, std::ref(polygon));
    auto cbinner = std::bind(&KmlParser::polygonInnerCallback, this, _1, std::ref(polygon));
    iterateOverChildrenElements(el, "outerBoundaryIs", cbouter);
    iterateOverChildrenElements(el, "innerBoundaryIs", cbinner);

    m_polygons.append(polygon);
}

void KmlParser::polygonOuterCallback(const QDomElement &el, KmlPolygon &polygon)
{
    auto cb = std::bind(&KmlParser::polygonCoordinatesCallback, this, _1, std::ref(polygon));
    iterateOverChildrenElements(el, "coordinates", cb);
}

void KmlParser::polygonInnerCallback(const QDomElement &el, KmlPolygon &polygon)
{
    auto cb = std::bind(&KmlParser::polygonHolesCallback, this, _1, std::ref(polygon));
    iterateOverChildrenElements(el, "coordinates", cb);
}

void KmlParser::polygonCoordinatesCallback(const QDomElement &el, KmlPolygon &polygon)
{
    auto coordinates = parseCoordinates(el.text());
    for (auto &c : coordinates)
        polygon.data.addCoordinate(c);
}

void KmlParser::polygonHolesCallback(const QDomElement &el, KmlPolygon &polygon)
{
    auto coordinates = parseCoordinates(el.text());
    polygon.data.addHole(coordinates);
}

QList<QGeoCoordinate> KmlParser::parseCoordinates(const QString &text)
{
    QList<QGeoCoordinate> result;
    QStringList tuples = text.simplified().split(" ", Qt::SkipEmptyParts);
    for (auto &t : tuples) {
        QStringList coordinates = t.split(",", Qt::SkipEmptyParts);
        bool ok1, ok2;
        double lon = t.section(",", 0, 0).toDouble(&ok1);
        double lat = t.section(",", 1, 1).toDouble(&ok2);
        if (ok1 && ok2) {
            result.append(QGeoCoordinate(lat, lon));
        } else
            apxMsgW() << "Can't parse lat-lon from string " << t;
    }
    return result;
}
