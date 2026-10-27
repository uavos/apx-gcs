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
#include "SimLink.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <XbusNode.h>

namespace fdm {

SimLink::SimLink(int index, const std::string &host, uint16_t port_ctr, uint16_t port_sns)
    : _index(index)
    , _name("sim:" + std::to_string(index))
    , _host(host)
    , _port_ctr(port_ctr)
    , _port_sns(port_sns)
    , _udp(_name.c_str(), true)
{}

SimLink::~SimLink()
{
    close();
}

bool SimLink::open()
{
    close();
    _udp.set_dest(_host.c_str(), _port_sns);
    if (!_udp.bind(_port_ctr)) {
        if (!_open_logged)
            log("failed to bind UDP port %u", _port_ctr);
        _open_logged = true;
        return false;
    }
    if (!_open_logged)
        log("controls from port %u, sensors to %s:%u", _port_ctr, _host.c_str(), _port_sns);
    _open_logged = true;
    return true;
}

void SimLink::close()
{
    _udp.close();
}

bool SimLink::connected() const
{
    return const_cast<xbus::tcp::udp_client &>(_udp).is_connected();
}

void SimLink::log(const char *fmt, ...) const
{
    va_list args;
    va_start(args, fmt);
    printf("[%s] ", _name.c_str());
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    fflush(stdout);
}

// --- rx

void SimLink::poll(double now)
{
    if (!_udp.is_connected())
        return;

    // bounded loop: never starve the physics on a flooding peer
    for (int guard = 0; guard < 64; ++guard) {
        if (!_udp.dataAvailable())
            break;

        size_t rcnt = _udp.read(_rx_buf, sizeof(_rx_buf));
        if (!rcnt)
            break;

        const uint8_t *data = _rx_buf;
        while (rcnt > 0) {
            auto dcnt = _rx_decoder.decode(data, rcnt);

            switch (_rx_decoder.status()) {
            case SerialDecoder::PacketAvailable:
                data += dcnt;
                parse_rx(_rx_decoder.data(), _rx_decoder.size(), now);
                break;
            case SerialDecoder::DataAccepted:
            case SerialDecoder::DataDropped:
                rcnt = 0;
                break;
            default:
                break;
            }

            if (rcnt > dcnt)
                rcnt -= dcnt;
            else
                rcnt = 0;
        }
    }
}

void SimLink::parse_rx(const void *data, size_t size, double now)
{
    XbusStreamReader stream(data, size);
    if (stream.available() < xbus::pid_s::psize())
        return;

    xbus::pid_s pid;
    pid.read(&stream);

    const mandala::uid_t uid = pid.uid;
    if (!mandala::cmd::env::sim::match(uid))
        return;

    switch (uid) {
    case mandala::cmd::env::sim::ctr::uid: {
        if (pid.pri != xbus::pri_final)
            break;
        _t_last_rx = now;
        _rx_ctr++;
        if (!_channels_valid)
            break;
        parse_ctr(stream);
    } break;

    case mandala::cmd::env::sim::cfg::uid: {
        if (pid.pri != xbus::pri_response)
            break;
        _t_last_rx = now;
        parse_cfg(stream);
    } break;

    default:
        break;
    }
}

void SimLink::parse_cfg(XbusStreamReader &stream)
{
    std::vector<std::string> names;
    while (names.size() < 32) {
        const char *s = stream.read_string(sizeof(xbus::node::conf::text_t));
        if (!s)
            break;
        names.emplace_back(s);
    }

    reset_channels();

    if (names.empty()) {
        log("controls config is empty");
        return;
    }

    log("controls config received (%zu channels)", names.size());
    for (const auto &name : names) {
        const Input in = map_channel(name);
        _channels.push_back(in);
        if (in == Input::None)
            log("  '%s' -> unknown input, ignored", name.c_str());
        else
            log("  '%s' -> %s", name.c_str(), input_name(in));
    }
    _channels_valid = true;
    _waiting_logged = false;
}

void SimLink::parse_ctr(XbusStreamReader &stream)
{
    for (size_t i = 0; i < _channels.size(); ++i) {
        if (stream.available() < sizeof(float))
            break;
        const float v = stream.read<float>();
        apply(_channels[i], v);
    }
}

void SimLink::apply(Input in, float v)
{
    if (!std::isfinite(v))
        return;
    switch (in) {
    case Input::Ail:
        _controls.ail = v;
        break;
    case Input::Elv:
        _controls.elv = v;
        break;
    case Input::Rud:
        _controls.rud = v;
        break;
    case Input::RudTrim:
        _controls.rud_trim = v;
        break;
    case Input::Thr:
        _controls.thr = v;
        break;
    case Input::Flaps:
        _controls.flaps = v;
        break;
    case Input::Brake:
        _controls.brake = v;
        break;
    case Input::Airbrake:
        _controls.airbrake = v;
        break;
    case Input::Collective:
        _controls.collective = v;
        break;
    case Input::TailRotor:
        _controls.rud = v;
        break;
    case Input::Launch:
        _controls.launch = v;
        break;
    case Input::Parachute:
        _controls.parachute = v;
        break;
    case Input::None:
        break;
    }
}

void SimLink::reset_channels()
{
    _channels.clear();
    _channels_valid = false;
    _controls = Controls();
}

// Channel names are X-Plane dataref names from the node config
// (controls.mixer.ch_xpl). Both the full dataref path and its last
// component are recognized so that existing unit configs work as is.
SimLink::Input SimLink::map_channel(const std::string &name_in)
{
    std::string name = name_in;

    // trim
    while (!name.empty() && isspace((unsigned char) name.back()))
        name.pop_back();
    size_t start = 0;
    while (start < name.size() && isspace((unsigned char) name[start]))
        start++;
    name = name.substr(start);

    // strip array index, keep its value: ENGN_thro_use[1] is the second engine
    int index = 0;
    const auto br = name.find('[');
    if (br != std::string::npos) {
        index = atoi(name.c_str() + br + 1);
        name = name.substr(0, br);
    }

    // last path component
    std::string base = name;
    const auto slash = name.rfind('/');
    if (slash != std::string::npos)
        base = name.substr(slash + 1);

    struct Alias
    {
        const char *name;
        Input input;
    };
    static const Alias aliases[] = {
        {"yoke_roll_ratio", Input::Ail},
        {"yoke_pitch_ratio", Input::Elv},
        {"yoke_heading_ratio", Input::Rud},
        {"rud_trim", Input::RudTrim},
        {"ENGN_thro_use", Input::Thr},
        {"ENGN_thro", Input::Thr},
        {"throttle_ratio", Input::Thr},
        {"throttle_ratio_all", Input::Thr},
        {"flaprqst", Input::Flaps},
        {"flaprat", Input::Flaps},
        {"flap_ratio", Input::Flaps},
        {"parkbrake", Input::Brake},
        {"parking_brake_ratio", Input::Brake},
        {"l_brake_add", Input::Brake},
        {"r_brake_add", Input::Brake},
        {"sbrkrqst", Input::Airbrake},
        {"sbrkrat", Input::Airbrake},
        {"speedbrake_ratio", Input::Airbrake},
        {"POINT_pitch_deg", Input::Collective}, // helicopter collective [deg]
        {"launch", Input::Launch},              // uavos/launch
        {"parachute", Input::Parachute},        // uavos/parachute
    };

    for (const auto &a : aliases) {
        if (base != a.name)
            continue;
        // the second engine of a helicopter is the electric tail rotor
        if (a.input == Input::Thr && index == 1)
            return Input::TailRotor;
        return a.input;
    }
    return Input::None;
}

const char *SimLink::input_name(Input in)
{
    switch (in) {
    case Input::Ail:
        return "aileron";
    case Input::Elv:
        return "elevator";
    case Input::Rud:
        return "rudder+steering";
    case Input::RudTrim:
        return "rudder trim";
    case Input::Thr:
        return "throttle";
    case Input::Flaps:
        return "flaps";
    case Input::Brake:
        return "brake";
    case Input::Airbrake:
        return "airbrake";
    case Input::Collective:
        return "collective";
    case Input::TailRotor:
        return "tail rotor";
    case Input::Launch:
        return "launch";
    case Input::Parachute:
        return "parachute";
    case Input::None:
        break;
    }
    return "none";
}

// --- tx

void SimLink::send_packet(XbusStreamWriter &stream)
{
    if (!_udp.is_connected())
        return;
    const auto cnt = _tx_encoder.encode(stream.buffer(), stream.pos());
    _udp.write(_tx_encoder.data(), cnt);
}

void SimLink::request(xbus::pid_s &pid)
{
    XbusStreamWriter stream(_tx_buf, sizeof(_tx_buf));
    pid.write(&stream);
    send_packet(stream);
    pid.seq++;
}

void SimLink::send_sensors(const mandala::bundle::sim_s &sns)
{
    XbusStreamWriter stream(_tx_buf, sizeof(_tx_buf));
    _pid_sns.write(&stream);
    stream.write(&sns, sizeof(sns));
    send_packet(stream);
    _pid_sns.seq++;
    _tx_sns++;
}

void SimLink::tick(double now)
{
    if (!_udp.is_connected()) {
        if (now - _t_last_bind >= kBindRetryInterval) {
            _t_last_bind = now;
            open();
        }
        return;
    }

    // controls stopped coming - the node restarted or changed config
    if (_channels_valid && _t_last_rx >= 0 && now - _t_last_rx > kLinkTimeout) {
        log("controls link lost");
        reset_channels();
    }

    if (!_channels_valid && now - _t_last_cfg_req >= kCfgRequestInterval) {
        _t_last_cfg_req = now;
        if (!_waiting_logged) {
            _waiting_logged = true;
            log("waiting for controls config from SIL node...");
        }
        request(_pid_cfg);
    }
}

} // namespace fdm
