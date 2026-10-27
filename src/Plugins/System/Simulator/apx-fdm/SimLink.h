/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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

#include <cstdint>
#include <string>
#include <vector>

#include <MandalaBundles.h>
#include <MandalaMetaTree.h>
#include <XbusPacket.h>
#include <XbusStreamReader.h>
#include <XbusStreamWriter.h>

#include <serial/CobsDecoder.h>
#include <serial/CobsEncoder.h>
#include <udp_client.h>

#include "Aircraft.h"

namespace fdm {

// UDP link to one `sim` SIL node instance.
//
// Protocol (same as the X-Plane plugin, see ../xplane/plugin.cpp):
//  - packets are xbus (pid + payload) framed with COBS over UDP;
//  - the simulator listens for controls on port_ctr and sends sensors
//    to host:port_sns;
//  - `cmd.env.sim.cfg` request -> response with the list of control
//    channel names (X-Plane dataref names from the node config);
//  - `cmd.env.sim.ctr` (final) carries one float per configured channel;
//  - `cmd.env.sim.sns` (final) carries mandala::bundle::sim_s.
class SimLink
{
public:
    SimLink(int index, const std::string &host, uint16_t port_ctr, uint16_t port_sns);
    ~SimLink();

    SimLink(const SimLink &) = delete;
    SimLink &operator=(const SimLink &) = delete;

    bool open();
    void close();

    // read and parse all pending datagrams (non blocking)
    void poll(double now);

    // periodic housekeeping: config requests, link timeout, re-bind
    void tick(double now);

    void send_sensors(const mandala::bundle::sim_s &sns);

    const Controls &controls() const { return _controls; }
    bool configured() const { return _channels_valid; }
    bool connected() const;
    int index() const { return _index; }

    uint64_t rx_ctr_count() const { return _rx_ctr; }
    uint64_t tx_sns_count() const { return _tx_sns; }

private:
    enum class Input : uint8_t {
        None,
        Ail,
        Elv,
        Rud,
        RudTrim,
        Thr,
        Flaps,
        Brake,
        Airbrake,
        Collective, // helicopter rotor blade pitch [deg]
        TailRotor,  // helicopter tail rotor (yaw)
        Launch,     // catapult release
        Parachute,  // parachute deploy
    };

    // map an X-Plane dataref name to a model input
    static Input map_channel(const std::string &name);
    static const char *input_name(Input in);

    void parse_rx(const void *data, size_t size, double now);
    void parse_cfg(XbusStreamReader &stream);
    void parse_ctr(XbusStreamReader &stream);
    void apply(Input in, float v);
    void reset_channels();

    void request(xbus::pid_s &pid);
    void send_packet(XbusStreamWriter &stream);

    void log(const char *fmt, ...) const;

    const int _index;
    const std::string _name;
    const std::string _host;
    const uint16_t _port_ctr;
    const uint16_t _port_sns;

    xbus::tcp::udp_client _udp;
    CobsDecoder<> _rx_decoder;
    CobsEncoder<> _tx_encoder;
    uint8_t _rx_buf[xbus::size_packet_max * 4];
    uint8_t _tx_buf[xbus::size_packet_max];

    std::vector<Input> _channels;
    bool _channels_valid{false};
    Controls _controls;

    double _t_last_rx{-1.0};
    double _t_last_cfg_req{-1.0e9};
    double _t_last_bind{-1.0e9};
    bool _waiting_logged{false};
    bool _open_logged{false};

    uint64_t _rx_ctr{};
    uint64_t _tx_sns{};

    xbus::pid_s _pid_sns{mandala::cmd::env::sim::sns::uid, xbus::pri_final, 0};
    xbus::pid_s _pid_cfg{mandala::cmd::env::sim::cfg::uid, xbus::pri_request, 0};

    static constexpr double kCfgRequestInterval{2.0}; // [s]
    static constexpr double kLinkTimeout{2.0};        // [s] without controls -> re-request config
    static constexpr double kBindRetryInterval{1.0};  // [s]
};

} // namespace fdm
