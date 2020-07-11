#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include <algorithm>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
//---------------
#include <version.h>

#include <Mandala/MandalaBundles.h>
#include <Mandala/MandalaMetaTree.h>

#include <Xbus/XbusNode.h>
#include <Xbus/XbusPacket.h>

#include <Xbus/tcp/tcp_server.h>
#include <tcp_ports.h>

#include <mathlib/mathlib.h>
#include <matrix/math.hpp>

using namespace math;
using namespace matrix;

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

static xbus::pid_s sim_pid{mandala::cmd::env::sim::uid, xbus::pri_final, 0};

#define PWM_CNT 10
static xbus::node::conf::text_t xpl[PWM_CNT]; //var names per PWM channel
static bool xpl_is_array[PWM_CNT];
static uint xpl_array_idx[PWM_CNT];
static XPLMDataRef xpl_ref[PWM_CNT];
static XPLMDataTypeID xpl_type[PWM_CNT];
static float pwm_ch[PWM_CNT]; //received pwm_ch values

static void send_bundle()
{
    static uint8_t buf[xbus::size_packet_max];
    XbusStreamWriter stream(buf, sizeof(buf));
    sim_pid.write(&stream);
    stream.write(&sim_bundle, sizeof(sim_bundle));

    tcp.write_packet(buf, stream.pos());
}

void parse_sensors(void)
{
    float elapsed = XPLMGetElapsedTime();
    static float elapsed_s = 0;
    float dt = elapsed - elapsed_s;
    elapsed_s = elapsed;

    // AHRS

    Vector3f att_rad = {wrap_pi(radians((float) XPLMGetDataf(xp.roll))),
                        wrap_pi(radians((float) XPLMGetDataf(xp.pitch))),
                        wrap_pi(radians((float) XPLMGetDataf(xp.yaw)))};
    att_rad.copyTo(sim_bundle.att);

    Quatf dq{Eulerf(att_rad)};

    Vector3f acc_ned = {
        -(float) XPLMGetDataf(xp.az),        //N=-vz
        +(float) XPLMGetDataf(xp.ax),        //E=vx
        -(float) XPLMGetDataf(xp.ay) - 9.81f //D=-vy
    };

    Vector3f acc = dq.conjugate_inversed(acc_ned);

    acc.copyTo(sim_bundle.acc);

    Vector3f gyro_rad = {radians((float) XPLMGetDataf(xp.p)),
                         radians((float) XPLMGetDataf(xp.q)),
                         radians((float) XPLMGetDataf(xp.r))};

    gyro_rad.copyTo(sim_bundle.gyro);

    Vector3f llh = {wrap_pi(radians((float) XPLMGetDatad(xp.lat))),
                    wrap_pi(radians((float) XPLMGetDatad(xp.lon))),
                    (float) XPLMGetDatad(xp.alt)};

    /*Vector3f xyz = {(float) XPLMGetDatad(xp.x),
                    (float) XPLMGetDatad(xp.y),
                    (float) XPLMGetDatad(xp.z)};
    double lat, lon, hmsl;
    XPLMLocalToWorld(xyz(0), xyz(1), xyz(2), &lat, &lon, &hmsl);
    llh = {math::radians((float) lat), math::radians((float) lon), (float) hmsl};*/
    llh.copyTo(sim_bundle.llh);

    Vector3f vel_ned = {
        -(float) XPLMGetDataf(xp.vz), //N=-vz
        +(float) XPLMGetDataf(xp.vx), //E=vx
        -(float) XPLMGetDataf(xp.vy)  //D=vy
    };
    vel_ned.copyTo(sim_bundle.vel);

    // AGL
    sim_bundle.agl = (float) XPLMGetDataf(xp.agl);

    // RPM
    static float a[8];
    XPLMGetDatavf(xp.rpm, a, 0, 1);
    sim_bundle.rpm = degrees(a[0] * 60.f) / 360.f;

    // Airdata
    sim_bundle.airspeed = 0.51444f * (float) XPLMGetDataf(xp.airspeed); //knots to mps

    float rho = (float) XPLMGetDataf(xp.rho);
    float at = (float) XPLMGetDataf(xp.air_temp);
    sim_bundle.baro_mbar = rho * 287.1f * (at + 273.15f) / 100.0f;
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

    //server.tcpdebug = true;
    //server.connect(0, TCP_PORT_SIM, false, "/sim");
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

    //request xpl config
    //uint8_t v = idx_sim;
    //server.write(&v, 1);

    /*xp.ail = XPLMFindDataRef("sim/joystick/yoke_roll_ratio");
  //xp.ail = XPLMFindDataRef("sim/flightmodel/controls/wing1l_ail1def");
  xp.elv= XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
  xp.rud = XPLMFindDataRef("sim/joystick/yoke_heading_ratio");
  //xp.thr = XPLMFindDataRef("sim/joystick/joystick_axis_values");
  xp.thr = XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro_use");
  xp.flap = XPLMFindDataRef("sim/flightmodel/controls/flaprqst");
  xp.brakes = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");

  xp.dsp = XPLMFindDataRef("sim/operation/override/override_planepath");
  xp.x = XPLMFindDataRef("sim/flightmodel/position/local_x");
  xp.y = XPLMFindDataRef("sim/flightmodel/position/local_y");
  xp.z = XPLMFindDataRef("sim/flightmodel/position/local_z");
*/
    XPLMRegisterFlightLoopCallback(flightLoopCallback, 1.0, nullptr);
    return 1;
}

static float flightLoopCallback(float inElapsedSinceLastCall,
                                float inElapsedTimeSinceLastFlightLoop,
                                int inCounter,
                                void *inRefcon)
{
    if (!enabled)
        return 1.0;

    parse_sensors();
    send_bundle();

    //send controls to sim
    /*uint rcnt = 0;
    while (1) {
        static uint8_t buf[xbus::size_packet_max];
        uint cnt = server.read(buf, sizeof(buf));
        if (!cnt)
            break;
        XbusStreamReader stream(buf);
        if (cnt <= sizeof(xbus::pid_t))
            continue;
        xbus::pid_t pid;
        stream >> pid;
        if (pid != idx_sim)
            continue;
        uint8_t op;
        stream >> op;

        switch (op) {
        case 0: //controls assignments
            printf("X-Plane controls assignments for PWM updated.\n");
            memset(&xpl_ref, 0, sizeof(xpl_ref));
            memset(&xpl_is_array, 0, sizeof(xpl_is_array));
            memset(&xpl_array_idx, 0, sizeof(xpl_array_idx));
            stream.read(xpl, sizeof(xpl));
            for (uint i = 0; i < PWM_CNT; ++i) {
                xbus::node::conf::ft_lstr_t &s = xpl[i];
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
            break;
        case 1: //PWM outputs
            if (rcnt)
                break; //repeated packet, skip
            stream.read(pwm_ch, sizeof(pwm_ch));
            for (uint i = 0; i < PWM_CNT; i++) {
                if (!xpl_ref[i])
                    continue;
                //printf("set: %s=%f\n",(char*)xpl[i],(pwm_ch[i]));
                if (xpl_is_array[i]) {
                    XPLMSetDatavf(xpl_ref[i], &(pwm_ch[i]), xpl_array_idx[i], 1);
                    //                    if (xpl_type[i] & xplmType_FloatArray)
                    //                        XPLMSetDatavf(xpl_ref[i], &(pwm_ch[i]), xpl_array_idx[i], 1);
                    //                    else if (xpl_type[i] & xplmType_IntArray) {
                    //                        int v = pwm_ch[i];
                    //                        XPLMSetDatavi(xpl_ref[i], &v, xpl_array_idx[i], 1);
                    //                    }
                    continue;
                }
                XPLMSetDataf(xpl_ref[i], pwm_ch[i]);
//                if (xpl_type[i] & xplmType_Float)
//                    XPLMSetDataf(xpl_ref[i], pwm_ch[i]);
//                else if (xpl_type[i] & xplmType_Double)
//                    XPLMSetDatad(xpl_ref[i], pwm_ch[i]);
//                else if (xpl_type[i] & xplmType_Int)
//                    XPLMSetDatai(xpl_ref[i], pwm_ch[i]);
//                else
//                    XPLMSetDataf(xpl_ref[i], pwm_ch[i]);
            }
            break;
        case 2: //Mandala var
            //var.extract(&packet->data[1],cnt-1);
            break;
        } //switch
        rcnt++;
    } //while rcv
    //if(bDisplay)return 0.01;
    //send vars to shiva
    sendVars();*/
    return -1.0; //call me next loop
}

PLUGIN_API void XPluginStop(void)
{
    enabled = false;
}
PLUGIN_API void XPluginDisable(void)
{
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"),0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"),0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"),0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"),0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 0);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 0);

    //server.close();

    enabled = false;
}
PLUGIN_API int XPluginEnable(void)
{
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 1);

    enabled = true;
    return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam) {}
