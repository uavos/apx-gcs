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

#include <Fact/Fact.h>
#include <QtCore>
#include <QtNetwork>

#include <serial/CobsDecoder.h>
#include <serial/CobsEncoder.h>

class Simulator;

class SimSwarm : public Fact
{
    Q_OBJECT

public:
    explicit SimSwarm(Simulator *parent);

private:
    Simulator *_sim;

    Fact *f_enb;
    Fact *f_port;
    Fact *f_peers;

    // bus stream of the master instance in, swarm bundles out
    QUdpSocket _udp;
    QList<QPair<QHostAddress, quint16>> _peers;

    CobsDecoder<> _dec;
    CobsEncoder<> _enc;

    // recently sent packets, to break loops through sims
    // retransmitting received bundles to their bus
    QElapsedTimer _time;
    QList<QPair<qint64, QByteArray>> _sent;

    void udpBind();
    void updatePeers();
    void sendToSims(const QByteArray &packet);

    void dropExpired();
    bool wasSent(const QByteArray &packet);

private slots:
    void readMaster();
};
