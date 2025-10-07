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
#include "DatalinkSerialRemotes.h"

#include "Datalink.h"
#include "DatalinkSerialRemoteTcp.h"
#include "DatalinkSerialRemoteUdpMcast.h"
#include <App/AppLog.h>
#include <App/AppSettings.h>

DatalinkSerialRemotes::DatalinkSerialRemotes(Datalink *datalink)
    : Fact(datalink,
           "remote_serials",
           tr("Remote serial ports"),
           "",
           Group | FlatModel,
           "serial-port")
    , m_datalink(datalink)
{
    //connect to specific host menu
    f_add = new Fact(this,
                     "add",
                     tr("Connect to remote serial port"),
                     tr("Create new connection"),
                     Group);
    f_add->setIcon("plus");
    f_host = new Fact(f_add,
                      "host",
                      tr("Host"),
                      tr("TCP Address of remote serial port"),
                      Text | PersistentValue,
                      "domain");
    f_port = new Fact(f_add,
                      "port",
                      tr("Port"),
                      tr("TCP Port of remote serial port"),
                      Int | PersistentValue,
                      "numeric");
    f_type = new Fact(f_add,
                      "type",
                      tr("Type"),
                      tr("Type of connection"),
                      Enum | PersistentValue,
                      "format-list-bulleted-type");
    f_type->setEnumStrings({"tcp", "udp_mcast"});
    f_list = new Fact(this,
                      "remote_ports",
                      tr("Remote ports"),
                      tr("Configured remote ports"),
                      Section | Count);
    f_connect = new Fact(f_add, "connect", tr("Connect"), "", Action | Apply | CloseOnTrigger);
    connect(f_connect, &Fact::triggered, this, &DatalinkSerialRemotes::onConnectTriggered);

    load();
}

void DatalinkSerialRemotes::updateStatus()
{
    int connectedCount = 0;
    int totalCount = f_list->size();
    for (auto f : f_list->facts()) {
        if (f->active()) {
            connectedCount++;
        }
    }
    setValue(QString("%1/%2").arg(connectedCount).arg(totalCount));
}

void DatalinkSerialRemotes::load()
{
    QFile file(AppDirs::prefs().filePath("remote_ports.json"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(file.readAll());
        if (!json.isEmpty() && !json.isNull()) {
            for (const auto &v : json.array()) {
                auto o = v.toObject();
                QString host = o["host"].toString();
                int port = o["port"].toInt();
                QString type = o["type"].toString();
                if (!type.isEmpty()) {
                    auto c = createConnection(host, port, type);
                    c->setActivated(o["activated"].toBool());
                }
            }
        }
    }
}
void DatalinkSerialRemotes::save()
{
    QFile file(AppDirs::prefs().filePath("remote_ports.json"));
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        auto objs = f_list->facts();
        QJsonArray array;
        for (auto o : objs) {
            auto tcpc = qobject_cast<DatalinkSerialRemoteTcp *>(o);
            if (tcpc) {
                QJsonObject json;
                json["host"] = tcpc->getHost();
                json["port"] = tcpc->getPort();
                json["activated"] = tcpc->activated();
                json["type"] = "tcp";
                array.append(json);
            }
            auto udpmcastc = qobject_cast<DatalinkSerialRemoteUdpMcast *>(o);
            if (udpmcastc) {
                QJsonObject json;
                json["host"] = udpmcastc->getHost();
                json["port"] = udpmcastc->getPort();
                json["activated"] = udpmcastc->activated();
                json["type"] = "udp_mcast";
                array.append(json);
            }
        }
        file.write(QJsonDocument(array).toJson());
        file.close();
    } else {
        apxMsgW() << file.errorString();
    }
}

DatalinkConnection *DatalinkSerialRemotes::createConnection(const QString &host,
                                                            int port,
                                                            const QString &type)
{
    DatalinkConnection *c = nullptr;
    if (type == "tcp") {
        DatalinkSerialRemoteTcp *tcpc = new DatalinkSerialRemoteTcp(f_list, host, port);
        connect(tcpc->f_remove,
                &Fact::triggered,
                this,
                &DatalinkSerialRemotes::onConnectionRemoveTriggered);
        c = tcpc;
    } else if (type == "udp_mcast") {
        DatalinkSerialRemoteUdpMcast *udpc = new DatalinkSerialRemoteUdpMcast(f_list, host, port);
        connect(udpc->f_remove,
                &Fact::triggered,
                this,
                &DatalinkSerialRemotes::onConnectionRemoveTriggered);
        c = udpc;
    }
    m_datalink->addConnection(c);
    c->setValue(true);

    connect(c,
            &DatalinkSerialRemoteTcp::activeChanged,
            this,
            &DatalinkSerialRemotes::onConnectionActiveChanged);
    connect(c, &DatalinkSerialRemoteTcp::activatedChanged, this, &DatalinkSerialRemotes::save);

    updateStatus();
    return c;
}

void DatalinkSerialRemotes::onConnectionActiveChanged()
{
    auto c = qobject_cast<DatalinkSerialRemoteTcp *>(sender());
    updateStatus();
}

void DatalinkSerialRemotes::onConnectionRemoveTriggered()
{
    save();
    updateStatus();
}

void DatalinkSerialRemotes::onConnectTriggered()
{
    createConnection(f_host->text(), f_port->value().toInt(), f_type->text());
    save();
}
