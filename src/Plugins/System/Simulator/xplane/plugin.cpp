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
#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include <algorithm>
#include <cstring>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <MandalaBundles.h>
#include <MandalaMetaTree.h>

#include <XbusNode.h>
#include <XbusPacket.h>

#include <tcp_ports.h>

#include <udp_client.h>

#include <serial/CobsDecoder.h>
#include <serial/CobsEncoder.h>

static xbus::tcp::udp_client udp;

static CobsDecoder<> _rx_decoder;
static CobsEncoder<> _tx_encoder;

static bool enabled;

static struct
{
    //input
    XPLMDataRef roll, pitch, yaw;
    XPLMDataRef ax, ay, az;
    XPLMDataRef x, y, z;
    XPLMDataRef p, q, r;
    XPLMDataRef psi;
    XPLMDataRef lat, lon, alt;
    XPLMDataRef vx, vy, vz;
    XPLMDataRef bearing;
    XPLMDataRef airspeed;
    XPLMDataRef agl;
    XPLMDataRef rpm;
    XPLMDataRef altitude;
    XPLMDataRef rho;
    XPLMDataRef air_temp;
    XPLMDataRef room_temp;
    XPLMDataRef slip;
    //output
    /*
    XPLMDataRef ail,elv,thr,rud,flap,brakes;
    //display
    XPLMDataRef dsp;
    XPLMDataRef x,y,z;
    */

} xp;

static mandala::bundle::sim_s sim_bundle{};

static xbus::pid_s sim_pid{mandala::cmd::env::sim::sns::uid, xbus::pri_final, 0};
static xbus::pid_s cfg_pid{mandala::cmd::env::sim::cfg::uid, xbus::pri_request, 0};

static xbus::pid_s usr_pid_f{mandala::cmd::env::sim::usr::uid, xbus::pri_final, 0};
static xbus::pid_s usr_pid_r{mandala::cmd::env::sim::usr::uid, xbus::pri_request, 0};
struct XplChannel
{
    xbus::node::conf::text_t xpl;
    uint idx;
    bool is_array;
    XPLMDataRef ref;
    XPLMDataTypeID type;

    void clear()
    {
        idx = 0;
        is_array = false;
        ref = nullptr;
        type = 0;
    }

    void set_data(float val)
    {
        if (!ref) {
            return;
        }

        if (is_array) {
            XPLMSetDatavf(ref, &val, idx, 1);
        } else {
            XPLMSetDataf(ref, val);
        }
    }

    float get_data()
    {
        if (!ref) {
            return 0.f;
        }

        float val = 0.f;

        switch (type) {
        case xplmType_Int: {
            val = XPLMGetDatai(ref);
        } break;
        case xplmType_Float: {
            val = XPLMGetDataf(ref);
        } break;
        case xplmType_Double: {
            val = (float) XPLMGetDatad(ref);
        } break;
        case xplmType_FloatArray: {
            XPLMGetDatavf(ref, &val, idx, 1);
        } break;
        case xplmType_IntArray: {
            int v = 0;
            XPLMGetDatavi(ref, &v, idx, 1);
            val = (float) v;
        } break;
        default:
            break;
        }

        return val;
    }
};
struct XplChannels
{
    XplChannel *channels = nullptr;
    uint8_t count_channels;
    uint8_t count_valid{0};

    explicit XplChannels(uint8_t count)
        : count_channels(count)
    {
        channels = new XplChannel[count_channels];
    }

    void clear()
    {
        for (uint8_t i = 0; i < count_channels; ++i) {
            channels[i].clear();
        }
    }
};

//commands
static XplChannels xpl_channels{16};

//user data
static XplChannels xpl_users{24};

static uint8_t packet_buf[xbus::size_packet_max];

static void send_packet(XbusStreamWriter *stream)
{
    if (!udp.is_connected())
        return;

    auto cnt = _tx_encoder.encode(stream->buffer(), stream->pos());
    udp.write(_tx_encoder.data(), cnt);
}

static void request(xbus::pid_s &_pid)
{
    XbusStreamWriter stream(packet_buf, sizeof(packet_buf));
    _pid.write(&stream);
    send_packet(&stream);
    _pid.seq++;
}

static void send_sns_bundle()
{
    XbusStreamWriter stream(packet_buf, sizeof(packet_buf));
    sim_pid.write(&stream);
    stream.write(&sim_bundle, sizeof(sim_bundle));
    send_packet(&stream);
    sim_pid.seq++;
}

static void send_users_bundle()
{
    XbusStreamWriter stream(packet_buf, sizeof(packet_buf));
    usr_pid_f.write(&stream);
    stream << xpl_users.count_valid;
    for (uint8_t i = 0; i < xpl_users.count_valid; ++i) {
        float val = xpl_users.channels[i].get_data();
        stream << val;
    }
    send_packet(&stream);
    usr_pid_f.seq++;
}

void parse_sensors(void)
{
    float elapsed = XPLMGetElapsedTime();
    static float elapsed_s = 0;
    float dt = elapsed - elapsed_s;
    elapsed_s = elapsed;

    // AHRS
    sim_bundle.att_deg[0] = XPLMGetDataf(xp.roll);
    sim_bundle.att_deg[1] = XPLMGetDataf(xp.pitch);
    sim_bundle.att_deg[2] = XPLMGetDataf(xp.yaw);

    sim_bundle.gyro_deg[0] = XPLMGetDataf(xp.p);
    sim_bundle.gyro_deg[1] = XPLMGetDataf(xp.q);
    sim_bundle.gyro_deg[2] = XPLMGetDataf(xp.r);

    sim_bundle.acc_ned[0] = -XPLMGetDataf(xp.az);
    sim_bundle.acc_ned[1] = +XPLMGetDataf(xp.ax);
    sim_bundle.acc_ned[2] = -XPLMGetDataf(xp.ay) - 9.81f;

    sim_bundle.lat_deg = XPLMGetDatad(xp.lat);
    sim_bundle.lon_deg = XPLMGetDatad(xp.lon);
    sim_bundle.hmsl = XPLMGetDataf(xp.alt);

    sim_bundle.vel_ned[0] = -XPLMGetDataf(xp.vz);
    sim_bundle.vel_ned[1] = +XPLMGetDataf(xp.vx);
    sim_bundle.vel_ned[2] = -XPLMGetDataf(xp.vy);

    // AGL
    sim_bundle.agl = XPLMGetDataf(xp.agl);

    // RPM
    static float a[8];
    XPLMGetDatavf(xp.rpm, a, 0, 1);
    sim_bundle.rpm = (a[0] * 60.f) / (M_PI * 2.f);

    // Airdata
    sim_bundle.airspeed = 0.51444f * (float) XPLMGetDataf(xp.airspeed); //knots to mps

    float rho = (float) XPLMGetDataf(xp.rho);
    float at = (float) XPLMGetDataf(xp.air_temp);
    sim_bundle.baro_mbar = rho * 287.1f * (at + 273.15f) / 100.0f;

    sim_bundle.slip = XPLMGetDataf(xp.slip);
}

static void process_xpl_channels(XbusStreamReader *stream, XplChannels *data)
{
    uint8_t count = data->count_channels;
    uint8_t &ch = data->count_valid;

    for (ch = 0; ch < count; ++ch) {
        uint8_t cp_size = sizeof(data->channels[0].xpl);
        const char *s = stream->read_string(cp_size);
        if (!s) {
            break;
        }
        strncpy(data->channels[ch].xpl, s, cp_size);
    }

    printf("X-Plane controls updated (%u)\n", data->count_valid);

    data->clear();

    if (!data->count_valid) {
        return;
    }

    for (uint8_t i = 0; i < data->count_valid; ++i) {
        xbus::node::conf::text_t &s = data->channels[i].xpl;
        if (s[0] == 0)
            continue;
        char *it1 = strchr(s, '[');
        if (it1) {
            *it1++ = 0;
            char *it2 = strchr(it1, ']');
            if (!it2)
                continue;
            if (*(it2 + 1) != 0)
                continue;
            *it2 = 0;
            uint idx;
            if (sscanf(it1, "%u", &idx) != 1)
                continue;
            data->channels[i].idx = idx;
            data->channels[i].is_array = true;
        }
        XPLMDataRef ref = XPLMFindDataRef(s);
        if (!ref)
            continue;
        data->channels[i].type = XPLMGetDataRefTypes(ref);
        data->channels[i].ref = ref;
    }
}

static void parse_rx(const void *data, size_t size)
{
    XbusStreamReader stream(data, size);
    if (stream.available() < xbus::pid_s::psize())
        return;
    xbus::pid_s pid;
    pid.read(&stream);

    mandala::uid_t uid = pid.uid;
    if (!mandala::cmd::env::sim::match(uid))
        return;

    switch (uid) {
    case mandala::cmd::env::sim::ctr::uid: {
        if (pid.pri != xbus::pri_final)
            break;
        if (!xpl_channels.count_valid)
            break;
        for (uint8_t i = 0; i < xpl_channels.count_valid; i++) {
            float v = stream.read<float>();
            xpl_channels.channels[i].set_data(v);
        }
    } break;
    case mandala::cmd::env::sim::cfg::uid: {
        if (pid.pri != xbus::pri_response)
            break;
        process_xpl_channels(&stream, &xpl_channels);
    } break;
    case mandala::cmd::env::sim::usr::uid: {
        if (pid.pri != xbus::pri_response)
            break;
        process_xpl_channels(&stream, &xpl_users);
    } break;
    }
}

static float flightLoopCallback(float inElapsedSinceLastCall,
                                float inElapsedTimeSinceLastFlightLoop,
                                int inCounter,
                                void *inRefcon);

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    printf("--------------------------------------\n");
    printf("APX X-Plane plugin version: %s\n", VERSION);
    printf("--------------------------------------\n");
    fflush(stdout);

    strcpy(outName, "UAVOS Autopilot SIL plugin");
    strcpy(outSig, "www.uavos.com");
    strcpy(outDesc, "Exchange data with autopilot for SIL/HIL simulation (v" VERSION ").");
    fflush(stdout);

    xpl_channels.clear();
    xpl_users.clear();

    xp.roll = XPLMFindDataRef("sim/flightmodel/position/phi");
    xp.pitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    xp.yaw = XPLMFindDataRef("sim/flightmodel/position/psi");

    xp.ax = XPLMFindDataRef("sim/flightmodel/position/local_ax");
    xp.ay = XPLMFindDataRef("sim/flightmodel/position/local_ay");
    xp.az = XPLMFindDataRef("sim/flightmodel/position/local_az");

    xp.x = XPLMFindDataRef("sim/flightmodel/position/local_x");
    xp.y = XPLMFindDataRef("sim/flightmodel/position/local_y");
    xp.z = XPLMFindDataRef("sim/flightmodel/position/local_z");

    xp.p = XPLMFindDataRef("sim/flightmodel/position/P");
    xp.q = XPLMFindDataRef("sim/flightmodel/position/Q");
    xp.r = XPLMFindDataRef("sim/flightmodel/position/R");

    xp.psi = XPLMFindDataRef("sim/flightmodel/position/magpsi");

    xp.lat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    xp.lon = XPLMFindDataRef("sim/flightmodel/position/longitude");
    xp.alt = XPLMFindDataRef("sim/flightmodel/position/elevation");

    xp.bearing = XPLMFindDataRef("sim/flightmodel/position/hpath");

    xp.vx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
    xp.vy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
    xp.vz = XPLMFindDataRef("sim/flightmodel/position/local_vz");

    xp.airspeed = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
    xp.agl = XPLMFindDataRef("sim/flightmodel/position/y_agl");

    xp.rpm = XPLMFindDataRef("sim/flightmodel/engine/ENGN_tacrad");

    xp.altitude = XPLMFindDataRef("sim/flightmodel/misc/h_ind2");
    xp.rho = XPLMFindDataRef("sim/weather/rho");
    xp.air_temp = XPLMFindDataRef("sim/weather/temperature_ambient_c");

    //xp.slip = XPLMFindDataRef("sim/flightmodel/misc/slip");
    xp.slip = XPLMFindDataRef("sim/cockpit2/gauges/indicators/sideslip_degrees");

    //xp.dsp = XPLMFindDataRef("sim/operation/override/override_planepath");
    //xp.x = XPLMFindDataRef("sim/flightmodel/position/local_x");
    //xp.y = XPLMFindDataRef("sim/flightmodel/position/local_y");
    //xp.z = XPLMFindDataRef("sim/flightmodel/position/local_z");

    XPLMRegisterFlightLoopCallback(flightLoopCallback, 1.0, nullptr);
    return 1;
}

static float flightLoopCallback(float inElapsedSinceLastCall,
                                float inElapsedTimeSinceLastFlightLoop,
                                int inCounter,
                                void *inRefcon)
{
    float now = XPLMGetElapsedTime();
    static float channels_timeout = 0.f;
    static float users_timeout = 0.f;
    static float users_pub_timeout = 0.f;
    static uint ap_link_timeout = 0;

    if (ap_link_timeout++ > 3) {
        ap_link_timeout = 0;
        xpl_channels.count_valid = 0; // reset channels assignments
        xpl_users.count_valid = 0;
    }

    if (!(enabled && udp.is_connected())) {
        xpl_channels.count_valid = 0;
        xpl_users.count_valid = 0;
        ap_link_timeout = 0;
        return 1.f;
    }

    do {
        for (;;) {
            if (!udp.dataAvailable())
                break;

            ap_link_timeout = 0;

            // printf("udp.dataAvailable\n");

            size_t rcnt = udp.read(packet_buf, sizeof(packet_buf));
            if (!rcnt)
                break;

            // printf("udp.read: %lu\n", rcnt);

            const uint8_t *data = packet_buf;
            while (rcnt > 0) {
                auto dcnt = _rx_decoder.decode(data, rcnt);

                switch (_rx_decoder.status()) {
                case SerialDecoder::PacketAvailable: {
                    data = static_cast<const uint8_t *>(data) + dcnt;
                    parse_rx(_rx_decoder.data(), _rx_decoder.size());
                    break;
                }

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

        if (now - channels_timeout > 2.f && !xpl_channels.count_valid) {
            channels_timeout = now;
            printf("X-Plane controls request\n");
            request(cfg_pid);
        }

        if (now - users_timeout > 3.f && !xpl_users.count_valid) {
            users_timeout = now;
            printf("X-Plane user data request\n");
            request(usr_pid_r);
        }

    } while (0);

    float loop = 2.f;
    if (xpl_channels.count_valid) {
        parse_sensors();
        send_sns_bundle();
        loop = -1.f;
    }

    //10Hz
    if (now - users_pub_timeout > 0.1f && xpl_users.count_valid) {
        users_pub_timeout = now;
        send_users_bundle();
        loop = -1.f;
    }

    return loop; //call me next loop
}

PLUGIN_API void XPluginStop(void)
{
    udp.close();
    enabled = false;
}

PLUGIN_API void XPluginDisable(void)
{
    printf("X-Plane plugin disabled\n");

    udp.close();

    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"), 0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"), 0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"), 0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"), 0);

    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 0);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 0);

    enabled = false;
}

PLUGIN_API int XPluginEnable(void)
{
    printf("X-Plane plugin enabled\n");

    if (!udp.is_connected()) {
        udp.set_dest("127.0.0.1", UDP_PORT_AP_SNS);
        udp.bind(UDP_PORT_SIM_CTR);
    }

    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"), 1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"), 1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"), 1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"), 1);

    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 1);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 1);

    enabled = true;
    return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam) {}
