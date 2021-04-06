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
#include "DatalinkPorts.h"
#include "Datalink.h"
#include "DatalinkPort.h"
#include <App/AppLog.h>
#include <App/AppSettings.h>
//=============================================================================
DatalinkPorts::DatalinkPorts(Datalink *datalink)
    : Fact(datalink,
           "ports",
           tr("Local ports"),
           tr("Modems and persistent remotes"),
           Group | FlatModel,
           "usb")
    , datalink(datalink)
{
    f_add = new DatalinkPort(this, datalink);
    f_add->setIcon("plus-circle");

    f_list = new Fact(this, "ports", tr("Ports"), tr("Configured ports"), Section | Count);
    connect(f_list, &Fact::sizeChanged, this, &DatalinkPorts::updateStatus);

    load();

    updateStatus();
}
//=============================================================================
void DatalinkPorts::updateStatus()
{
    int cnt = f_list->size();
    if (cnt <= 0) {
        setValue("");
        setActive(false);
    } else {
        int ecnt = 0, acnt = 0;
        for (int i = 0; i < cnt; ++i) {
            Fact *f = f_list->child(i);
            if (f->value().toBool())
                ecnt++;
            if (f->active())
                acnt++;
        }
        setValue(QString("%1/%2/%3").arg(acnt).arg(ecnt).arg(cnt));
        setActive(acnt > 0);
    }
}
//=============================================================================
void DatalinkPorts::addTriggered()
{
    addPort(new DatalinkPort(this, datalink, f_add));
    save();
    f_add->defaults();
}
//=============================================================================
void DatalinkPorts::addPort(DatalinkPort *port)
{
    connect(port, &Fact::valueChanged, this, &DatalinkPorts::updateStatus);
    connect(port, &Fact::activeChanged, this, &DatalinkPorts::updateStatus);
}
//=============================================================================
void DatalinkPorts::load()
{
    QFile file(AppDirs::prefs().filePath("ports.json"));
    if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(file.readAll());
        file.close();
        for (auto const v : json.array()) {
            f_add->clear();
            f_add->fromVariant(v.toObject().toVariantMap());
            addPort(new DatalinkPort(this, datalink, f_add));
        }
        f_add->defaults();
    }
}
void DatalinkPorts::save()
{
    QFile file(AppDirs::prefs().filePath("ports.json"));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << file.errorString();
        return;
    }
    file.write(f_list->toJsonDocument().toJson());
    file.close();
}
//=============================================================================
QStringList DatalinkPorts::activeSerialPorts() const
{
    QStringList st;
    foreach (DatalinkPort *port, serialPorts()) {
        if (!port->active())
            continue;
        st.append(port->f_connection->title());
    }
    return st;
}
//=============================================================================
QList<DatalinkPort *> DatalinkPorts::serialPorts() const
{
    QList<DatalinkPort *> list;
    for (auto i : f_list->facts()) {
        DatalinkPort *port = qobject_cast<DatalinkPort *>(i);
        if (!port)
            continue;
        if (port->f_type->value().toInt() != DatalinkPort::SERIAL)
            continue;
        list.append(port);
    }
    return list;
}
//=============================================================================
void DatalinkPorts::blockSerialPorts()
{
    for (auto port : serialPorts()) {
        if (!port->f_enable->value().toBool())
            continue;
        blockedPorts.append(port);
        port->f_enable->setValue(false);
    }
    if (blockedPorts.isEmpty())
        return;
    qDebug() << "Ports blocked" << blockedPorts.size();
}
void DatalinkPorts::unblockSerialPorts()
{
    if (blockedPorts.isEmpty())
        return;
    for (auto port : serialPorts()) {
        if (!blockedPorts.contains(port))
            continue;
        port->f_enable->setValue(true);
    }
    qDebug() << "Ports unblocked" << blockedPorts.size();
    blockedPorts.clear();
}
//=============================================================================
