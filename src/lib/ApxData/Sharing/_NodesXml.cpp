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
#include "NodesXml.h"
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Database/VehiclesReqDict.h>
#include <Database/VehiclesReqNconf.h>
#define NODES_XML_FORMAT 1
//=============================================================================
NodesXmlExport::NodesXmlExport(QString hash, QString title, QString fileName)
    : ShareXmlExport(Database::instance()->vehicles, "nodes", NODES_XML_FORMAT, title, fileName)
    , req(hash)
{}
bool NodesXmlExport::run(QSqlQuery &query)
{
    if (!req.run(query))
        return false;
    info = req.configInfo;

    //find vehicle info
    quint64 vehicleID = info.value("vehicleID").toULongLong();
    if (vehicleID) {
        query.prepare("SELECT * FROM Vehicles WHERE key=?");
        query.addBindValue(vehicleID);
        if (!query.exec())
            return false;
        if (query.next()) {
            vehicleInfo = filterIdValues(queryRecord(query));
        }
    }

    //collect node users for each node
    for (int i = 0; i < req.data.size(); ++i) {
        DBReqVehiclesLoadUser reqUser(
            req.data.at(i).value("dictInfo").value<QVariantMap>().value("sn").toString());
        if (!reqUser.run(query))
            return false;
        req.data[i]["userInfo"] = QVariant::fromValue(reqUser.info);
    }

    return ShareXmlExport::run(query);
}
bool NodesXmlExport::write(QDomNode &dom)
{
    QDomDocument doc = dom.ownerDocument();

    writeInfo(dom, "vehicle", vehicleInfo);

    for (int i = 0; i < req.data.size(); ++i) {
        writeNode(dom, req.data.at(i));
    }
    return true;
}
//=============================================================================
void NodesXmlExport::writeNode(QDomNode &dom, const QVariantMap &data) const
{
    //qDebug()<<data;
    const QVariantMap &dictInfo = data.value("dictInfo").value<QVariantMap>();
    if (dictInfo.isEmpty()) {
        qWarning() << "missing node dict info";
        return;
    }

    const QVariantMap &nconfInfo = data.value("nconfInfo").value<QVariantMap>();
    if (nconfInfo.isEmpty()) {
        qWarning() << "missing node config info";
        return;
    }

    const QVariantMap &userInfo = data.value("userInfo").value<QVariantMap>();

    QDomDocument doc = dom.ownerDocument();
    QDomNode e = dom.appendChild(doc.createElement("node"));
    e.toElement().setAttribute("name", dictInfo.value("name").toString());
    e.toElement().setAttribute("sn", dictInfo.value("sn").toString());

    //NodeDicts db fields
    writeInfo(e,
              "dictionary",
              dictInfo,
              QStringList() << "sn"
                            << "name");
    writeInfo(e,
              "config",
              nconfInfo,
              QStringList() << "sn"
                            << "name"
                            << "version"
                            << "hardware");
    writeInfo(e, "user", userInfo);

    const DictNode::Dict &dict = data.value("dict").value<DictNode::Dict>();
    if (!dict.commands.isEmpty()) {
        QDomNode e1 = e.appendChild(doc.createElement("commands"));
        e1.toElement().setAttribute("cnt", QString::number(dict.commands.size()));
        for (int i = 0; i < dict.commands.size(); i++) {
            const DictNode::Command &c = dict.commands.at(i);
            QDomNode e2 = e1.appendChild(doc.createElement("command"));
            e2.toElement().setAttribute("id", QString::number(c.cmd));
            e2.toElement().setAttribute("name", c.name);
            e2.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(c.descr));
        }
    }

    QDomNode e1 = e.appendChild(doc.createElement("fields"));
    e1.toElement().setAttribute("cnt", QString::number(dict.fields.size()));

    const QVariantMap &values = data.value("values").value<QVariantMap>();
    for (int i = 0; i < dict.fields.size(); ++i) {
        writeNodeField(e1, dict.fields.at(i), values);
    }
}
void NodesXmlExport::writeNodeField(QDomNode &dom,
                                    const DictNode::Field &f,
                                    const QVariantMap &values) const
{
    QDomDocument doc = dom.ownerDocument();
    QDomNode e = dom.appendChild(doc.createElement("field"));
    e.toElement().setAttribute("id", QString::number(f.id));
    e.toElement().setAttribute("name", f.name);
    e.appendChild(doc.createElement("type"))
        .appendChild(doc.createTextNode(DictNode::dataTypeToString(f.type)));
    e.appendChild(doc.createElement("title")).appendChild(doc.createTextNode(f.title));
    if (!f.descr.isEmpty())
        e.appendChild(doc.createElement("descr")).appendChild(doc.createTextNode(f.descr));
    if (!f.units.isEmpty())
        e.appendChild(doc.createElement("units")).appendChild(doc.createTextNode(f.units));
    if (!f.opts.isEmpty())
        e.appendChild(doc.createElement("opts")).appendChild(doc.createTextNode(f.opts.join(',')));
    if (!f.groups.isEmpty())
        e.appendChild(doc.createElement("sect")).appendChild(doc.createTextNode(f.groups.join('/')));

    //value
    if (!f.subFields.isEmpty()) {
        for (int i = 0; i < f.subFields.size(); ++i) {
            writeNodeField(e, f.subFields.at(i), values);
        }
    } else if (values.contains(f.name)) {
        QString v = values.value(f.name).toString();
        e.appendChild(doc.createElement("value")).appendChild(doc.createTextNode(v));
    } else {
        qWarning() << "missing value" << f.name;
    }
}
//=============================================================================
//=============================================================================
//=============================================================================
NodesXmlImport::NodesXmlImport(QString title, QString fileName)
    : ShareXmlImport(Database::instance()->vehicles, "nodes", NODES_XML_FORMAT, title, fileName)
{}
bool NodesXmlImport::read(const QDomNode &dom)
{
    vehicleInfo = readInfo(dom, "vehicle");

    for (QDomElement e = dom.firstChildElement("node"); !e.isNull();
         e = e.nextSiblingElement(e.tagName())) {
        QVariantMap nodesItem;

        QVariantMap dictInfo = readInfo(e, "dictionary");
        const QString sn = e.attribute("sn");
        dictInfo["sn"] = sn;
        dictInfo["name"] = e.attribute("name");
        nodesItem.insert("dictInfo", QVariant::fromValue(dictInfo));

        QVariantMap nconfInfo = readInfo(e, "config");
        nodesItem.insert("nconfInfo", QVariant::fromValue(nconfInfo));

        QVariantMap userInfo = readInfo(e, "user");
        nodesItem.insert("userInfo", QVariant::fromValue(userInfo));

        DictNode::Dict dict;
        QVariantMap values;

        //read commands
        QDomElement g = e.firstChildElement("commands");
        int gcnt = g.attribute("cnt").toInt();
        for (QDomElement e = g.firstChildElement("command"); !e.isNull();
             e = e.nextSiblingElement(e.tagName())) {
            uint cmd = e.attribute("id").toUInt();
            QString sname = e.attribute("name");
            QString sdescr = e.firstChildElement("descr").text();
            if (!(sname.isEmpty() || sdescr.isEmpty())) {
                DictNode::Command c;
                c.cmd = cmd;
                c.name = sname;
                c.descr = sdescr;
                dict.commands.append(c);
            }
        }
        if (dict.commands.size() != gcnt) {
            qWarning() << "inconsistent commands" << dict.commands.size() << gcnt;
            continue;
        }
        //read fields
        g = e.firstChildElement("fields");
        gcnt = g.attribute("cnt").toInt();
        for (QDomElement e = g.firstChildElement("field"); !e.isNull();
             e = e.nextSiblingElement(e.tagName())) {
            quint16 id = static_cast<quint16>(e.attribute("id").toUInt());
            QString r_name = e.toElement().attribute("name");
            if (r_name.isEmpty())
                continue;
            DictNode::Field f;
            f.id = id;
            f.type = DictNode::dataTypeFromString(e.firstChildElement("type").text());
            f.name = r_name;
            f.title = e.firstChildElement("title").text();
            f.descr = e.firstChildElement("descr").text();
            f.units = e.firstChildElement("units").text();
            f.opts = e.firstChildElement("opts").text().split(',', Qt::SkipEmptyParts);
            f.groups = e.firstChildElement("sect").text().split('/', Qt::SkipEmptyParts);
            f.valid = true;
            QDomElement ev = e.firstChildElement("value");
            if (!ev.isNull()) {
                QString v = ev.text();
                values.insert(f.name, v);
            }
            for (QDomElement ev = e.firstChildElement("field"); !ev.isNull();
                 ev = ev.nextSiblingElement(ev.tagName())) {
                DictNode::Field fs;
                fs.id = static_cast<quint16>(ev.attribute("id").toUInt());
                fs.type = DictNode::dataTypeFromString(ev.firstChildElement("type").text());
                fs.name = ev.attribute("name");
                fs.title = ev.firstChildElement("title").text();
                fs.descr = ev.firstChildElement("descr").text();
                fs.units = ev.firstChildElement("units").text();
                fs.opts = ev.firstChildElement("opts").text().split(',', Qt::SkipEmptyParts);
                fs.valid = true;
                f.subFields.append(fs);
                QDomElement evs = ev.firstChildElement("value");
                if (!evs.isNull()) {
                    QString v = evs.text();
                    values.insert(fs.name, v);
                }
            }
            dict.fields.append(f);
        }
        if (gcnt <= 0 || dict.fields.size() != gcnt) {
            qWarning() << "inconsistent fields" << dict.commands.size() << gcnt;
            continue;
        }
        dict.fieldsValid = true;
        dict.commandsValid = true;
        nodesItem.insert("dict", QVariant::fromValue(dict));
        nodesItem.insert("values", QVariant::fromValue(values));
        nodes.append(nodesItem);
    }
    return true;
}
bool NodesXmlImport::save(QSqlQuery &query)
{
    //register vehicle
    quint64 vehicleID = 1;        //LOCAL
    vehicleInfo.remove("squawk"); //mark imported
    QString uid = vehicleInfo.value("uid").toString();
    if (uid == QString(uid.size(), QChar('0'))) {
        uid.clear();
        vehicleInfo["uid"] = uid;
    }
    if (uid.isEmpty()) {
        vehicleInfo.remove("callsign");
    } else {
        DBReqSaveVehicleInfo req(vehicleInfo);
        if (!req.run(query))
            return false;
        vehicleID = req.vehicleID;
    }

    if (!vehicleID) {
        qWarning() << "Can't register vehicle" << vehicleInfo;
        return false;
    }
    //register nodes
    QList<quint64> nconfList;
    int ncnt = 0;
    for (int i = 0; i < nodes.size(); ++i) {
        const QVariantMap &nodesItem = nodes.at(i);
        QVariantMap dictInfo = nodesItem.value("dictInfo").value<QVariantMap>();
        //register node
        {
            QVariantMap ninfo = dictInfo;
            ninfo.remove("time");
            DBReqVehiclesSaveInfo req(ninfo);
            if (!req.run(query))
                break;
        }
        //register dictionary
        {
            DBReqVehiclesSaveDict req(dictInfo, nodesItem.value("dict").value<DictNode::Dict>());
            if (!req.run(query))
                break;
            dictInfo["key"] = req.info.value("key");
        }
        //register node config
        {
            const QVariantMap &nconfInfo = nodesItem.value("nconfInfo").value<QVariantMap>();
            const QVariantMap &values = nodesItem.value("values").value<QVariantMap>();
            DBReqVehiclesSaveNconf req(dictInfo, values, nconfInfo.value("time").toULongLong());
            if (!req.run(query))
                break;
            nconfList.append(req.nconfID);
        }
        //update node user
        {
            QVariantMap userInfo = nodesItem.value("userInfo").value<QVariantMap>();
            if (!userInfo.isEmpty()) {
                DBReqVehiclesSaveUser req(dictInfo.value("sn").toString(),
                                          userInfo,
                                          userInfo.value("time").toLongLong());
                if (!req.run(query))
                    break;
            }
        }

        //save config bundle
        if (i == (nodes.size() - 1)) {
            DBReqVehiclesSaveConfig req(nconfList,
                                        vehicleID,
                                        info.value("notes").toString(),
                                        info.value("time").toULongLong());
            if (!req.run(query))
                break;
            info["hash"] = req.configInfo.value("hash");
            info["key"] = req.configInfo.value("key");
            info["title"] = req.configInfo.value("title");
        }
        //update vehicle callsign to nodes config title

        ncnt++;
    }
    return ncnt == nodes.size();
}
//=============================================================================
