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
#include "Datalink.h"

#include <App/App.h>
//=============================================================================
Datalink::Datalink(Fact *parent)
    : Fact(parent, "datalink", tr("Datalink"), tr("Communication and networks"), Group)
    , bReadOnly(false)
    , m_valid(false)
    , m_online(false)
    , m_errcnt(0)
{
    setIcon("swap-vertical");

    f_readonly = new Fact(this,
                          "readonly",
                          tr("Read only"),
                          tr("Block all uplink data"),
                          Bool | PersistentValue,
                          "airplane-off");
    connect(f_readonly, &Fact::valueChanged, this, &Datalink::readonlyChanged);
    readonlyChanged();

    f_hbeat = new Fact(this,
                       "hbeat",
                       tr("Send heartbeat"),
                       tr("Vehicle datalink available status"),
                       Bool | PersistentValue,
                       "heart-circle-outline");
    f_hbeat->setDefaultValue(true);

    QString sect;
    sect = tr("Connections");

    f_server = new DatalinkServer(this);
    f_server->setSection(sect);
    f_remotes = new DatalinkRemotes(this);
    f_remotes->setSection(sect);
    f_ports = new DatalinkPorts(this);
    f_ports->setSection(sect);
    f_stats = new DatalinkStats(this);
    f_stats->setSection(sect);

    //heartbeat timer
    connect(f_hbeat, &Fact::valueChanged, this, &Datalink::hbeatChanged);
    connect(&heartbeatTimer, SIGNAL(timeout()), this, SLOT(heartbeatTimeout()));
    heartbeatTimer.setInterval(1500);
    hbeatChanged();

    //online timeout
    onlineTimer.setSingleShot(true);
    onlineTimer.setInterval(7000);
    connect(&onlineTimer, &QTimer::timeout, this, [=]() { setOnline(false); });
    connect(this, &Datalink::packetReceived, this, [=]() { setOnline(true); });
    connect(this, &Datalink::onlineChanged, this, [=]() {
        if (!m_online)
            setErrcnt(0);
        App::sound(m_online ? "connected" : "error");
    });

    connect(this, &Datalink::packetReceived, this, [=]() { setValid(true); });

    App::jsync(this);
}
//=============================================================================
void Datalink::addConnection(DatalinkConnection *c)
{
    connections.append(c);
    connect(c, &DatalinkConnection::packetReceived, this, &Datalink::connectionPacketReceived);
    connect(c, &Fact::removed, this, [this, c]() {
        //qDebug()<<"rm"<<c;
        connections.removeOne(c);
        updateStatus();
    });
    connect(c, &Fact::activeChanged, this, &Datalink::updateStatus);
    updateStatus();
}
//=============================================================================
void Datalink::updateStatus()
{
    int cnt = connections.size();
    if (cnt <= 0) {
        setStatus("");
    } else {
        int acnt = 0;
        for (int i = 0; i < cnt; ++i) {
            DatalinkConnection *c = connections.at(i);
            if (!c) {
                connections.removeAt(i);
                updateStatus();
                return;
            }
            if (c->active())
                acnt++;
        }
        setStatus(QString("%1/%2").arg(acnt).arg(cnt));
        setActive(acnt > 0);
    }
}
//=============================================================================
void Datalink::readonlyChanged()
{
    bReadOnly = f_readonly->value().toBool();
    if (bReadOnly)
        apxMsg() << tr("Read only datalink");
    else
        apxMsg() << tr("Uplink allowed");
}
//=============================================================================
void Datalink::heartbeatTimeout()
{
    if (!f_hbeat->value().toBool())
        return;
    emit heartbeat();
}
void Datalink::hbeatChanged()
{
    if (f_hbeat->value().toBool())
        heartbeatTimer.start();
    else
        heartbeatTimer.stop();
}
//=============================================================================
//=============================================================================
void Datalink::connectionPacketReceived(QByteArray packet, quint16 network)
{
    //qDebug()<<"R"<<packet.toHex().toUpper();
    if (packet.isEmpty())
        return;
    if (!network)
        return;
    DatalinkConnection *src = qobject_cast<DatalinkConnection *>(sender());
    if (!src)
        return;
    for (int i = 0; i < connections.size(); ++i) {
        DatalinkConnection *c = connections.at(i);
        if (!c)
            continue;
        if (c == src)
            continue;

        c->sendPacket(packet, network);
    }
    emit packetReceived(packet);
    emit receivedDataEvent(packet.size());
}
void Datalink::sendPacket(QByteArray packet)
{
    //qDebug()<<"S"<<packet.toHex().toUpper();
    if (bReadOnly)
        return;
    if (packet.isEmpty())
        return;
    for (int i = 0; i < connections.size(); ++i) {
        DatalinkConnection *c = connections.at(i);
        if (!c)
            continue;
        c->sendPacket(packet, LOCAL);
    }
    emit transmittedDataEvent(packet.size());
}
//=============================================================================
//=============================================================================
bool Datalink::valid() const
{
    return m_valid;
}
void Datalink::setValid(const bool &v)
{
    if (m_valid == v)
        return;
    m_valid = v;
    emit validChanged();
}
bool Datalink::online() const
{
    return m_online;
}
void Datalink::setOnline(const bool &v)
{
    if (v)
        onlineTimer.start();
    if (m_online == v)
        return;
    m_online = v;
    emit onlineChanged();
}
uint Datalink::errcnt() const
{
    return m_errcnt;
}
void Datalink::setErrcnt(const uint &v)
{
    if (m_errcnt == v)
        return;
    m_errcnt = v;
    emit errcntChanged();
}
//=============================================================================
