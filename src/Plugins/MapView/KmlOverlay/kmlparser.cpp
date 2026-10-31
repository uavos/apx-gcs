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
#include <QBuffer>

namespace {

constexpr int kMaxWarnings = 20;

bool keyMatches(const QString &key, std::initializer_list<const char *> patterns)
{
    for (const char *p : patterns) {
        if (key.contains(QString::fromUtf8(p), Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool isTag(const QXmlStreamReader &xml, const char *name)
{
    return xml.name() == QLatin1String(name);
}

} // namespace

KmlParser::KmlParser()
    : m_polygonId(0)
    , m_warnings(0)
{}

void KmlParser::parse(const QByteArray &data, const ProgressCallback &progress)
{
    m_polygonId = 0;
    m_warnings = 0;
    m_polygons.clear();
    m_polygonStyles.clear();
    m_styles.clear();
    m_styleMaps.clear();

    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);

    QXmlStreamReader xml(&buffer);
    const qint64 total = qMax<qint64>(1, data.size());
    int lastPercent = -1;
    int placemarks = 0;

    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;

        if (isTag(xml, "Placemark")) {
            parsePlacemark(xml);
            if (progress && (++placemarks % 64) == 0) {
                const int percent = int(buffer.pos() * 100 / total);
                if (percent != lastPercent) {
                    lastPercent = percent;
                    progress(percent);
                }
            }
        } else if (isTag(xml, "Style")) {
            parseStyle(xml, true);
        } else if (isTag(xml, "StyleMap")) {
            parseStyleMap(xml);
        }
    }

    if (xml.hasError()) {
        apxMsgW() << QString("%1 at line %2").arg(xml.errorString()).arg(xml.lineNumber());
    }

    resolveColors();

    if (progress)
        progress(100);
}

QList<KmlPolygon> KmlParser::getPolygons()
{
    return m_polygons;
}

// consumes the rest of the current element including its end tag
void KmlParser::skipElement(QXmlStreamReader &xml)
{
    xml.skipCurrentElement();
}

// KML stores colors as aabbggrr hex
QColor KmlParser::parseKmlColor(QStringView text)
{
    text = text.trimmed();
    if (text.startsWith(u'#'))
        text = text.mid(1);
    if (text.size() != 8 && text.size() != 6)
        return {};

    bool ok = false;
    const uint v = text.toUInt(&ok, 16);
    if (!ok)
        return {};

    const int a = text.size() == 8 ? int((v >> 24) & 0xff) : 0xff;
    const int b = int((v >> 16) & 0xff);
    const int g = int((v >> 8) & 0xff);
    const int r = int(v & 0xff);
    return QColor(r, g, b, a);
}

QString KmlParser::styleIdFromUrl(const QString &url)
{
    const QString s = url.trimmed();
    return s.mid(s.indexOf('#') + 1);
}

// reads a <Style> element, PolyStyle color first, LineStyle color second
QColor KmlParser::readStyleColor(QXmlStreamReader &xml)
{
    QColor poly;
    QColor line;

    while (xml.readNextStartElement()) {
        const bool isPoly = isTag(xml, "PolyStyle");
        const bool isLine = isTag(xml, "LineStyle");
        if (!isPoly && !isLine) {
            skipElement(xml);
            continue;
        }
        while (xml.readNextStartElement()) {
            if (isTag(xml, "color")) {
                const QColor c = parseKmlColor(xml.readElementText());
                if (isPoly)
                    poly = c;
                else
                    line = c;
            } else {
                skipElement(xml);
            }
        }
    }

    return poly.isValid() ? poly : line;
}

void KmlParser::parseStyle(QXmlStreamReader &xml, bool shared)
{
    const QString id = xml.attributes().value("id").toString();
    const QColor color = readStyleColor(xml);
    if (shared && !id.isEmpty() && color.isValid())
        m_styles.insert(id, color);
}

void KmlParser::parseStyleMap(QXmlStreamReader &xml)
{
    const QString id = xml.attributes().value("id").toString();
    QString normal;

    while (xml.readNextStartElement()) {
        if (!isTag(xml, "Pair")) {
            skipElement(xml);
            continue;
        }
        QString key;
        QString url;
        while (xml.readNextStartElement()) {
            if (isTag(xml, "key"))
                key = xml.readElementText().trimmed();
            else if (isTag(xml, "styleUrl"))
                url = xml.readElementText();
            else if (isTag(xml, "Style")) {
                // inline style inside a pair, register under a synthetic id
                const QString inlineId = id + QStringLiteral("#pair");
                const QColor c = readStyleColor(xml);
                if (c.isValid())
                    m_styles.insert(inlineId, c);
                url = inlineId;
            } else
                skipElement(xml);
        }
        if (!url.isEmpty() && (normal.isEmpty() || key == QLatin1String("normal")))
            normal = styleIdFromUrl(url);
    }

    if (!id.isEmpty() && !normal.isEmpty())
        m_styleMaps.insert(id, normal);
}

// Zone attributes are not standardized in KML, collect them best effort:
// <name> of the placemark first, then ExtendedData fields by key name.
void KmlParser::parseExtendedData(QXmlStreamReader &xml, PlacemarkInfo &info)
{
    auto assign = [&info](const QString &key, const QString &value) {
        if (key.isEmpty() || value.isEmpty())
            return;
        if (info.name.isEmpty()
            && keyMatches(key, {"name", "ident", "designator", "code", "title"}))
            info.name = value;
        else if (info.upper.isEmpty() && keyMatches(key, {"upper", "top", "ceil", "max"}))
            info.upper = value;
        else if (info.lower.isEmpty() && keyMatches(key, {"lower", "bottom", "floor", "min"}))
            info.lower = value;
    };

    // <Data name="..."><value>..</value></Data> and <SchemaData><SimpleData name="..">..</SimpleData>
    while (xml.readNextStartElement()) {
        if (isTag(xml, "Data")) {
            const QString key = xml.attributes().value("name").toString();
            QString value;
            while (xml.readNextStartElement()) {
                if (isTag(xml, "value"))
                    value = xml.readElementText().simplified();
                else
                    skipElement(xml);
            }
            assign(key, value);
        } else if (isTag(xml, "SchemaData")) {
            while (xml.readNextStartElement()) {
                if (isTag(xml, "SimpleData")) {
                    const QString key = xml.attributes().value("name").toString();
                    assign(key, xml.readElementText().simplified());
                } else
                    skipElement(xml);
            }
        } else {
            skipElement(xml);
        }
    }
}

void KmlParser::parsePlacemark(QXmlStreamReader &xml)
{
    PlacemarkInfo info;

    // geometry may be nested in MultiGeometry, walk the whole subtree
    int depth = 1;
    while (depth > 0 && !xml.atEnd()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::EndElement) {
            --depth;
            continue;
        }
        if (token != QXmlStreamReader::StartElement)
            continue;

        if (depth == 1 && isTag(xml, "name")) {
            info.name = xml.readElementText().simplified();
        } else if (depth == 1 && isTag(xml, "styleUrl")) {
            info.styleId = styleIdFromUrl(xml.readElementText());
        } else if (depth == 1 && isTag(xml, "Style")) {
            info.color = readStyleColor(xml);
        } else if (depth == 1 && isTag(xml, "ExtendedData")) {
            parseExtendedData(xml, info);
        } else if (isTag(xml, "Polygon")) {
            parsePolygon(xml, info);
        } else {
            ++depth;
        }
    }
}

void KmlParser::parsePolygon(QXmlStreamReader &xml, const PlacemarkInfo &info)
{
    KmlPolygon polygon;
    polygon.color = info.color;
    polygon.name = info.name;
    polygon.upper = info.upper;
    polygon.lower = info.lower;
    polygon.id = m_polygonId++;

    bool outer = false;
    bool inner = false;
    int depth = 1;
    while (depth > 0 && !xml.atEnd()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::EndElement) {
            if (isTag(xml, "outerBoundaryIs"))
                outer = false;
            else if (isTag(xml, "innerBoundaryIs"))
                inner = false;
            --depth;
            continue;
        }
        if (token != QXmlStreamReader::StartElement)
            continue;

        if (isTag(xml, "outerBoundaryIs")) {
            outer = true;
            ++depth;
        } else if (isTag(xml, "innerBoundaryIs")) {
            inner = true;
            ++depth;
        } else if (isTag(xml, "coordinates")) {
            const QList<QGeoCoordinate> coordinates = parseCoordinates(xml.readElementText());
            if (inner)
                polygon.data.addHole(coordinates);
            else if (outer || polygon.data.size() == 0) {
                for (const auto &c : coordinates)
                    polygon.data.addCoordinate(c);
            }
        } else {
            ++depth;
        }
    }

    m_polygons.append(polygon);
    m_polygonStyles.append(info.styleId);
}

void KmlParser::resolveColors()
{
    for (qsizetype i = 0; i < m_polygons.size(); ++i) {
        KmlPolygon &polygon = m_polygons[i];
        if (!polygon.color.isValid()) {
            QString id = m_polygonStyles.at(i);
            if (m_styleMaps.contains(id))
                id = m_styleMaps.value(id);
            polygon.color = m_styles.value(id);
        }
        if (!polygon.color.isValid())
            polygon.color = QColor(Qt::red);
    }
    m_polygonStyles.clear();
}

QList<QGeoCoordinate> KmlParser::parseCoordinates(QStringView text)
{
    QList<QGeoCoordinate> result;
    const qsizetype n = text.size();
    qsizetype i = 0;

    while (i < n) {
        while (i < n && text.at(i).isSpace())
            ++i;
        const qsizetype start = i;
        while (i < n && !text.at(i).isSpace())
            ++i;
        if (i == start)
            break;

        const QStringView tuple = text.mid(start, i - start);
        const qsizetype c1 = tuple.indexOf(u',');
        const qsizetype c2 = c1 < 0 ? -1 : tuple.indexOf(u',', c1 + 1);

        bool ok1 = false;
        bool ok2 = false;
        double lon = 0;
        double lat = 0;
        if (c1 > 0) {
            lon = tuple.left(c1).toDouble(&ok1);
            lat = (c2 < 0 ? tuple.mid(c1 + 1) : tuple.mid(c1 + 1, c2 - c1 - 1)).toDouble(&ok2);
        }

        if (ok1 && ok2) {
            result.append(QGeoCoordinate(lat, lon));
        } else if (m_warnings < kMaxWarnings) {
            ++m_warnings;
            apxMsgW() << "Can't parse lat-lon from string " << tuple.toString();
        }
    }
    return result;
}
