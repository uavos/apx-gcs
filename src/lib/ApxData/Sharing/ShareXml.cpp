/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "ShareXml.h"
#include <App/AppBase.h>
#include <App/AppLog.h>
#include <Database/Database.h>
//=============================================================================
void ShareXml::writeInfo(QDomNode &dom, QString name, QVariantMap info, QStringList filter)
{
    QDomDocument doc = dom.ownerDocument();
    QDomNode e = name.isEmpty() ? dom : dom.appendChild(doc.createElement(name));
    foreach (QString key, info.keys()) {
        if (key == "key")
            continue;
        if (key.endsWith(QLatin1String("id"), Qt::CaseInsensitive)
            && (!key.endsWith(QLatin1String("uid"), Qt::CaseInsensitive)))
            continue;
        if (filter.contains(key))
            continue;
        QString s = info.value(key).toString();
        if (s.isEmpty())
            continue;
        e.appendChild(doc.createElement(key)).appendChild(doc.createTextNode(s));
    }
}
QVariantMap ShareXml::readInfo(const QDomNode &dom, QString name)
{
    QVariantMap info;
    QDomElement e = name.isEmpty() ? dom.toElement() : dom.firstChildElement(name);
    if (e.isNull())
        return info;
    for (QDomElement n = e.firstChildElement(); !n.isNull(); n = n.nextSiblingElement()) {
        info.insert(n.tagName(), n.text());
    }
    return info;
}
//=============================================================================
//=============================================================================
bool ShareXmlExport::run(QSqlQuery &query)
{
    Q_UNUSED(query)

    QDomDocument doc;
    if (!fileName.isEmpty()) {
        doc.appendChild(
            doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
    }

    QDomNode e = doc.appendChild(doc.createElement(tagName));
    e.toElement().setAttribute("href", "http://www.uavos.com/");
    e.toElement().setAttribute("format", QString::number(format));
    //general info (xml only)
    e.appendChild(doc.createElement("title")).appendChild(doc.createTextNode(title));
    e.appendChild(doc.createElement("timestamp"))
        .appendChild(
            doc.createTextNode(QDateTime::fromMSecsSinceEpoch(info.value("time").toLongLong())
                                   .toString(Qt::RFC2822Date)));
    e.appendChild(doc.createElement("exported"))
        .appendChild(doc.createTextNode(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    e.appendChild(doc.createElement("version")).appendChild(doc.createTextNode(AppBase::version()));

    writeInfo(e, "info", info);

    if (!write(e)) {
        apxMsgW() << tr("Export error") << title;
        return false;
    }
    QTextStream stream(&data, QIODevice::WriteOnly);
    doc.save(stream, 2);
    stream.flush();
    if (!data.isEmpty())
        emit exported(data, fileName);
    return true;
}
//=============================================================================
bool ShareXmlExport::write(QDomNode &dom)
{
    Q_UNUSED(dom)
    qDebug() << "not implemented";
    return false;
}
//=============================================================================
//=============================================================================
bool ShareXmlImport::run(QSqlQuery &query)
{
    if ((!fileName.isEmpty()) || data.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            apxMsgW() << tr("Cannot read file").append(":") << fileName
                      << QString("(%1)").arg(file.errorString());
            return true;
        }
        QFileInfo fi(fileName);
        defaultTime = (fi.birthTime().isValid() ? fi.birthTime() : fi.lastModified())
                          .toMSecsSinceEpoch();
        data = file.readAll();
    }
    return readData(query);
}
//=============================================================================
bool ShareXmlImport::readData(QSqlQuery &query)
{
    QDomDocument doc;
    if (!(doc.setContent(data) && readData(doc.documentElement()))) {
        apxMsgW() << tr("The file format is not correct") << QString("(%1)").arg(tagName);
        return true;
    }
    if (!save(query)) {
        apxMsgW() << tr("Failed to save imported data") << QString("(%1)").arg(tagName);
        return false;
    }
    emit imported(info.value("hash").toString(), title);
    return true;
}
bool ShareXmlImport::readData(const QDomNode &dom)
{
    QDomElement e = dom.toElement();
    int fmt = e.attribute("format").toInt();
    if (fmt < format) {
        qWarning() << tr("Importing old format") << QString("(%1)").arg(tagName);
        return readOldFormat(e, fmt);
    }
    //read current format
    if (e.tagName() != tagName) {
        qWarning() << "wrong document tag" << e.tagName() << tagName;
        return false;
    }
    info = readInfo(e, "info");
    if (info.isEmpty()) {
        return false;
    }
    QString r_title = e.firstChildElement("title").text();
    if (!r_title.isEmpty())
        title = r_title;
    return read(e);
}
//=============================================================================
bool ShareXmlImport::read(const QDomNode &dom)
{
    Q_UNUSED(dom)
    qDebug() << "not implemented";
    return true;
}
//=============================================================================
bool ShareXmlImport::readOldFormat(const QDomNode &dom, int fmt)
{
    Q_UNUSED(dom)
    Q_UNUSED(fmt)
    qDebug() << "not implemented";
    return true;
}
//=============================================================================
bool ShareXmlImport::save(QSqlQuery &query)
{
    Q_UNUSED(query)
    qDebug() << "not implemented";
    return true;
}
//=============================================================================
