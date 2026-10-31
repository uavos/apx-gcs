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
#pragma once

#include <functional>
#include <QColor>
#include <QGeoPolygon>
#include <QHash>
#include <QXmlStreamReader>

struct KmlPolygon
{
    uint64_t id;
    QColor color;
    QGeoPolygon data;
    QString name;  // zone identifier as written in the file
    QString upper; // upper limit text as written in the file
    QString lower; // lower limit text as written in the file
};

// Streaming KML parser: collects polygons with their style color and
// best effort zone attributes without building a DOM of the whole file.
class KmlParser
{
public:
    using ProgressCallback = std::function<void(int percent)>;

    KmlParser();
    void parse(const QByteArray &data, const ProgressCallback &progress = {});
    QList<KmlPolygon> getPolygons();

private:
    struct PlacemarkInfo
    {
        QColor color;
        QString styleId;
        QString name;
        QString upper;
        QString lower;
    };

    QList<KmlPolygon> m_polygons;
    QList<QString> m_polygonStyles; // style id per polygon, resolved after parsing
    QHash<QString, QColor> m_styles;
    QHash<QString, QString> m_styleMaps;
    uint64_t m_polygonId;
    int m_warnings;

    void parseStyle(QXmlStreamReader &xml, bool shared);
    void parseStyleMap(QXmlStreamReader &xml);
    void parsePlacemark(QXmlStreamReader &xml);
    void parseExtendedData(QXmlStreamReader &xml, PlacemarkInfo &info);
    void parsePolygon(QXmlStreamReader &xml, const PlacemarkInfo &info);
    void skipElement(QXmlStreamReader &xml);
    void resolveColors();

    QColor readStyleColor(QXmlStreamReader &xml);
    QList<QGeoCoordinate> parseCoordinates(QStringView text);
    static QColor parseKmlColor(QStringView text);
    static QString styleIdFromUrl(const QString &url);
};
