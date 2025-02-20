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

#include "SimMod.h"

class SimMods : public Fact
{
    Q_OBJECT

public:
    explicit SimMods(Fact *parent = nullptr);

private:
    Fact *f_enb;
    Fact *f_ext_enb;

    Fact *f_port_ap;

    Fact *f_port_jtx;
    Fact *f_port_jrx;
    Fact *f_timeout_jrx;

    SimMod *f_mod;

    // sim link
    QUdpSocket _udp_sim;
    QUdpSocket _udp_json;

    CobsDecoder<> _dec;
    CobsEncoder<> _enc;

    // tx to sim
    xbus::pid_s _pid_sim_sns;
    uint8_t _buf_sim_tx[xbus::size_packet_max];

    // JSON rx timeout
    QElapsedTimer _json_rx_time;

private slots:
    void simRead();
    void parse_sim_rx(const void *data, size_t size);

    void jsonRead();
    void forwardToAP(const void *data, size_t size);
};
