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

    f_list = new Fact(this, "list", tr("Ports"), tr("Configured ports"), Section | Const);
    connect(f_list, &Fact::sizeChanged, this, &DatalinkPorts::updateStatus);

    load();

    updateStatus();
}
//=============================================================================
void DatalinkPorts::updateStatus()
{
    int cnt = f_list->size();
    if (cnt <= 0) {
        setStatus("");
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
        setStatus(QString("%1/%2/%3").arg(acnt).arg(ecnt).arg(cnt));
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
        foreach (QJsonValue v, json["list"].toArray()) {
            f_add->clear();
            f_add->valuesFromJson(v.toObject());
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
    file.write(QJsonDocument(f_list->valuesToJson(true)).toJson());
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
    for (int i = 0; i < f_list->size(); ++i) {
        DatalinkPort *port = f_list->child<DatalinkPort>(i);
        if (port->f_type->value().toInt() != DatalinkPort::SERIAL)
            continue;
        list.append(port);
    }
    return list;
}
//=============================================================================
void DatalinkPorts::blockSerialPorts()
{
    foreach (DatalinkPort *port, serialPorts()) {
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
    foreach (DatalinkPort *port, serialPorts()) {
        if (!blockedPorts.contains(port))
            continue;
        port->f_enable->setValue(true);
    }
    qDebug() << "Ports unblocked" << blockedPorts.size();
    blockedPorts.clear();
}
//=============================================================================
