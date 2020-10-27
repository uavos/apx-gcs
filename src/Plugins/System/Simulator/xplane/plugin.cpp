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
//---------------
#include <version.h>

#include <mandala/MandalaBundles.h>
#include <mandala/MandalaMetaTree.h>

#include <xbus/XbusNode.h>
#include <xbus/XbusPacket.h>

#include <tcp_ports.h>
#include <xbus/tcp/tcp_server.h>

static xbus::tcp::Server tcp;

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
    XPLMDataRef course;
    XPLMDataRef airspeed;
    XPLMDataRef agl;
    XPLMDataRef rpm;
    XPLMDataRef altitude;
    XPLMDataRef rho;
    XPLMDataRef air_temp;
    XPLMDataRef room_temp;
    //output
    /*XPLMDataRef ail,elv,thr,rud,flap,brakes;
  //display
  XPLMDataRef dsp;
  XPLMDataRef x,y,z;*/

} xp;

static mandala::bundle::sim_s sim_bundle{};

static xbus::pid_s sim_pid{mandala::cmd::env::sim::sns::uid, xbus::pri_final, 0};
static xbus::pid_s cfg_pid{mandala::cmd::env::sim::cfg::uid, xbus::pri_request, 0};

static uint8_t xpl_channels{0};                        //assigned controls cnt
static constexpr const uint8_t xpl_channels_max{16};   //assigned controls cnt
static xbus::node::conf::text_t xpl[xpl_channels_max]; //var names per PWM channel

static bool xpl_is_array[xpl_channels_max];
static uint xpl_array_idx[xpl_channels_max];
static XPLMDataRef xpl_ref[xpl_channels_max];
static XPLMDataTypeID xpl_type[xpl_channels_max];

static uint8_t packet_buf[xbus::size_packet_max];

static void send_bundle()
{
    XbusStreamWriter stream(packet_buf, sizeof(packet_buf));
    sim_pid.write(&stream);
    stream.write(&sim_bundle, sizeof(sim_bundle));

    tcp.write_packet(stream.buffer(), stream.pos());
    sim_pid.seq++;
}

static void request_controls()
{
    XbusStreamWriter stream(packet_buf, sizeof(packet_buf));
    cfg_pid.write(&stream);
    tcp.write_packet(stream.buffer(), stream.pos());
    cfg_pid.seq++;
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
        if (!xpl_channels)
            break;
        for (uint8_t i = 0; i < xpl_channels; i++) {
            float v = stream.read<float>();
            if (!xpl_ref[i])
                continue;
            //printf("set: %s=%f\n",(char*)xpl[i],(pwm_ch[i]));
            if (xpl_is_array[i]) {
                XPLMSetDatavf(xpl_ref[i], &v, xpl_array_idx[i], 1);
                //                    if (xpl_type[i] & xplmType_FloatArray)
                //                        XPLMSetDatavf(xpl_ref[i], &(pwm_ch[i]), xpl_array_idx[i], 1);
                //                    else if (xpl_type[i] & xplmType_IntArray) {
                //                        int v = pwm_ch[i];
                //                        XPLMSetDatavi(xpl_ref[i], &v, xpl_array_idx[i], 1);
                //                    }
                continue;
            }
            XPLMSetDataf(xpl_ref[i], v);
            //                if (xpl_type[i] & xplmType_Float)
            //                    XPLMSetDataf(xpl_ref[i], pwm_ch[i]);
            //                else if (xpl_type[i] & xplmType_Double)
            //                    XPLMSetDatad(xpl_ref[i], pwm_ch[i]);
            //                else if (xpl_type[i] & xplmType_Int)
            //                    XPLMSetDatai(xpl_ref[i], pwm_ch[i]);
            //                else
            //                    XPLMSetDataf(xpl_ref[i], pwm_ch[i]);
        }
    } break;
    case mandala::cmd::env::sim::cfg::uid: {
        if (pid.pri != xbus::pri_response)
            break;
        for (xpl_channels = 0; xpl_channels < xpl_channels_max; ++xpl_channels) {
            const char *s = stream.read_string(sizeof(xpl[0]));
            if (!s)
                break;
            strncpy(xpl[xpl_channels], s, sizeof(xpl[0]));
        }
        printf("X-Plane controls updated (%u)\n", xpl_channels);
        memset(&xpl_ref, 0, sizeof(xpl_ref));
        memset(&xpl_is_array, 0, sizeof(xpl_is_array));
        memset(&xpl_array_idx, 0, sizeof(xpl_array_idx));
        if (!xpl_channels)
            break;
        for (uint8_t i = 0; i < xpl_channels; ++i) {
            xbus::node::conf::text_t &s = xpl[i];
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
                uint aidx;
                if (sscanf(it1, "%u", &aidx) != 1)
                    continue;
                //if(!(XPLMGetDataRefTypes(sref)&(xplmType_FloatArray|xplmType_IntArray)))continue;
                xpl_array_idx[i] = aidx;
                xpl_is_array[i] = true;
            }
            XPLMDataRef ref = XPLMFindDataRef(s);
            if (!ref)
                continue;
            xpl_type[i] = XPLMGetDataRefTypes(s);
            xpl_ref[i] = ref;
            printf("DataRef: %s\n", s);
        }
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

    tcp.set_host("127.0.0.1", TCP_PORT_SIM, "/sim");
    tcp.connect();

    strcpy(outName, "UAVOS Autopilot SIL plugin");
    strcpy(outSig, "www.uavos.com");
    sprintf(outDesc, "Exchange data with autopilot for SIL/HIL simulation (v%s).", VERSION);

    memset(&xpl, 0, sizeof(xpl));
    memset(&xpl_ref, 0, sizeof(xpl_ref));
    memset(&xpl_is_array, 0, sizeof(xpl_is_array));
    memset(&xpl_array_idx, 0, sizeof(xpl_array_idx));

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

    xp.course = XPLMFindDataRef("sim/flightmodel/position/hpath");

    xp.vx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
    xp.vy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
    xp.vz = XPLMFindDataRef("sim/flightmodel/position/local_vz");

    xp.airspeed = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
    xp.agl = XPLMFindDataRef("sim/flightmodel/position/y_agl");

    xp.rpm = XPLMFindDataRef("sim/flightmodel/engine/ENGN_tacrad");

    xp.altitude = XPLMFindDataRef("sim/flightmodel/misc/h_ind2");
    xp.rho = XPLMFindDataRef("sim/weather/rho");
    xp.air_temp = XPLMFindDataRef("sim/weather/temperature_ambient_c");

    //  xp.dsp = XPLMFindDataRef("sim/operation/override/override_planepath");
    //  xp.x = XPLMFindDataRef("sim/flightmodel/position/local_x");
    //  xp.y = XPLMFindDataRef("sim/flightmodel/position/local_y");
    //  xp.z = XPLMFindDataRef("sim/flightmodel/position/local_z");

    XPLMRegisterFlightLoopCallback(flightLoopCallback, 1.0, nullptr);
    return 1;
}

static float flightLoopCallback(float inElapsedSinceLastCall,
                                float inElapsedTimeSinceLastFlightLoop,
                                int inCounter,
                                void *inRefcon)
{
    if (!(enabled && tcp.is_connected())) {
        xpl_channels = 0;
        return 1.f;
    }

    do {
        while (1) {
            size_t cnt = tcp.read_packet(packet_buf, sizeof(packet_buf));
            if (!cnt)
                break;
            parse_rx(packet_buf, cnt);
        }

        if (!xpl_channels) {
            //xpl_channels = 1;
            request_controls();
            return 0.5f;
        }

    } while (0);

    parse_sensors();
    send_bundle();

    return -1.f; //call me next loop
}

PLUGIN_API void XPluginStop(void)
{
    enabled = false;
}
PLUGIN_API void XPluginDisable(void)
{
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"), 0);
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"), 0);
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"), 0);
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"), 0);

    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 0);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 0);

    //server.close();

    enabled = false;
}
PLUGIN_API int XPluginEnable(void)
{
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"), 1);
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"), 1);
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"), 1);
    //    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"), 1);

    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 1);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 1);

    enabled = true;
    return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam) {}
