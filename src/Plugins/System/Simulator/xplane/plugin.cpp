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

#include <Mandala/flat/Mandala.h>

#include <Xbus/XbusNodeConf.h>
#include <Xbus/XbusPacket.h>

#include <Xbus/tcp/tcp_server.h>
#include <tcp_ports.h>
//==============================================================================
//#define UDP_HOST_UAV            "127.0.0.1"      //machine where 'shiva' runs
static _tcp_server server;
//==============================================================================
_var_float noise(_var_float amp);
Vect noise_vect(_var_float amp);
//==============================================================================
static Mandala var;
static bool enabled;
//==============================================================================
static struct
{
    //input
    XPLMDataRef roll, pitch, yaw;
    XPLMDataRef ax, ay, az;
    XPLMDataRef p, q, r;
    XPLMDataRef psi;
    XPLMDataRef lat, lon, alt;
    XPLMDataRef vx, vy, vz;
    XPLMDataRef course;
    XPLMDataRef airspeed;
    XPLMDataRef agl;
    XPLMDataRef rpm;
    XPLMDataRef altitude;
    //output
    /*XPLMDataRef ail,elv,thr,rud,flap,brakes;
  //display
  XPLMDataRef dsp;
  XPLMDataRef x,y,z;*/

} ref;
//==============================================================================
#define PWM_CNT 10
static std::array<xbus::node::conf::ft_lstr_t, PWM_CNT> xpl; //var names per PWM channel
static bool xpl_is_array[PWM_CNT];
static uint xpl_array_idx[PWM_CNT];
static XPLMDataRef xpl_ref[PWM_CNT];
static XPLMDataTypeID xpl_type[PWM_CNT];
static std::array<float, PWM_CNT> pwm_ch; //received pwm_ch values
//==============================================================================
void sendvar(uint16_t idx)
{
    static uint8_t buf[xbus::size_packet_max];
    XbusStreamWriter stream(buf);
    stream.write<xbus::pid_t>(idx_sim);
    stream.write<xbus::pid_t>(idx);

    size_t cnt = var.pack(buf + stream.position(), idx);
    cnt += stream.position();
    server.write(buf, cnt);
}
//==============================================================================
void sendVars(void)
{
    _var_float elapsed = XPLMGetElapsedTime();
    static _var_float elapsed_s = 0;
    _var_float dt = elapsed - elapsed_s;
    elapsed_s = elapsed;

    var.theta[0] = var.boundAngle((_var_float) XPLMGetDataf(ref.roll));
    var.theta[1] = var.boundAngle((_var_float) XPLMGetDataf(ref.pitch));
    var.theta[2] = var.boundAngle((_var_float) XPLMGetDataf(ref.yaw));
    sendvar(idx_theta);

    //frame accelerations
    Vect accNED = Vect(-(_var_float) XPLMGetDataf(ref.az),       //N=-vz
                       +(_var_float) XPLMGetDataf(ref.ax),       //E=vx
                       -(_var_float) XPLMGetDataf(ref.ay) - 9.81 //D=-vy
    );

    var.acc = var.rotate(accNED, var.theta); //+noise_vect(0.3);

    var.gyro = Vect((_var_float) XPLMGetDataf(ref.p),
                    (_var_float) XPLMGetDataf(ref.q),
                    (_var_float) XPLMGetDataf(ref.r)); //+noise_vect(2);

    var.acc += noise_vect(0.5);
    var.gyro += noise_vect(1);
    sendvar(idx_acc);
    sendvar(idx_gyro);

    var.mag = var.rotate(Vect(1, 0, 0), var.theta);
    var.mag += noise_vect(0.05);
    sendvar(idx_mag);

    var.airspeed = 0.51444 * (_var_float) XPLMGetDataf(ref.airspeed); //knots to mps
    var.airspeed += noise(0.05);
    var.airspeed *= 0.895;
    sendvar(idx_airspeed);

    static float a[8];
    XPLMGetDatavf(ref.rpm, a, 0, 1);
    var.rpm = a[0] * 60.0 * R2D / 360.0 + noise(100);
    sendvar(idx_rpm);

    var.agl = 0.2 + (_var_float) XPLMGetDataf(ref.agl);
    /*if((float)rand()/(float)RAND_MAX>0.99 || var.agl>7){
    float d=10.0*(float)rand()/(float)(RAND_MAX)-5;
    if(std::abs(d)>1)var.agl+=d;
    if(var.agl<0)var.agl=0;
  }*/
    //if(var.power&power_agl)
    sendvar(idx_agl);

    var.gps_pos[0] = (_var_float) XPLMGetDatad(ref.lat);
    var.gps_pos[1] = (_var_float) XPLMGetDatad(ref.lon);
    var.gps_pos[2] = (_var_float) XPLMGetDatad(ref.alt);
    var.gps_vel[0] = -(_var_float) XPLMGetDataf(ref.vz); //N=-vz
    var.gps_vel[1] = +(_var_float) XPLMGetDataf(ref.vx); //E=vx
    var.gps_vel[2] = -(_var_float) XPLMGetDataf(ref.vy); //D=vy
    sendvar(idx_gps_pos);
    sendvar(idx_gps_vel);

    var.gps_SU = 12;
    sendvar(idx_gps_SU);

    //local altitude
    //if(var.gps_home_hmsl==0)var.gps_home_hmsl=var.gps_hmsl;
    //var.altitude=var.gps_hmsl-var.gps_home_hmsl;
    //var.altitude=0.3048*(_var_float)XPLMGetDataf(ref.altitude); //knots to mps
    var.altps = var.gps_pos[2]; //101325.0*std::pow(1.0-2.25577e-5*var.gps_pos[2],5.25588)/3386.389;
    var.altps += noise(0.01);
    sendvar(idx_altps);

    var.vario = -var.gps_vel[2];
    var.vario += noise(0.05);
    sendvar(idx_vario);
}
//==============================================================================
float flightLoopCallback(float inElapsedSinceLastCall,
                         float inElapsedTimeSinceLastFlightLoop,
                         int inCounter,
                         void *inRefcon);
//==============================================================================
PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    printf("--------------------------------------\n");
    printf("APX plugin version: %s\n", VERSION);
    printf("--------------------------------------\n");
    fflush(stdout);

    server.tcpdebug = true;
    server.connect(0, TCP_PORT_SIM, false, "/sim");

    strcpy(outName, "UAVOS Autopilot SIL plugin");
    strcpy(outSig, "www.uavos.com");
    sprintf(outDesc, "Exchange data with autopilot for SIL/HIL simulation (v%s).", VERSION);

    memset(&xpl, 0, sizeof(xpl));
    memset(&xpl_ref, 0, sizeof(xpl_ref));
    memset(&xpl_is_array, 0, sizeof(xpl_is_array));
    memset(&xpl_array_idx, 0, sizeof(xpl_array_idx));

    ref.roll = XPLMFindDataRef("sim/flightmodel/position/phi");
    ref.pitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    ref.yaw = XPLMFindDataRef("sim/flightmodel/position/psi");

    ref.ax = XPLMFindDataRef("sim/flightmodel/position/local_ax");
    ref.ay = XPLMFindDataRef("sim/flightmodel/position/local_ay");
    ref.az = XPLMFindDataRef("sim/flightmodel/position/local_az");

    ref.p = XPLMFindDataRef("sim/flightmodel/position/P");
    ref.q = XPLMFindDataRef("sim/flightmodel/position/Q");
    ref.r = XPLMFindDataRef("sim/flightmodel/position/R");

    ref.psi = XPLMFindDataRef("sim/flightmodel/position/magpsi");

    ref.lat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    ref.lon = XPLMFindDataRef("sim/flightmodel/position/longitude");
    ref.alt = XPLMFindDataRef("sim/flightmodel/position/elevation");

    ref.course = XPLMFindDataRef("sim/flightmodel/position/hpath");

    ref.vx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
    ref.vy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
    ref.vz = XPLMFindDataRef("sim/flightmodel/position/local_vz");

    ref.airspeed = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
    ref.agl = XPLMFindDataRef("sim/flightmodel/position/y_agl");

    ref.rpm = XPLMFindDataRef("sim/flightmodel/engine/ENGN_tacrad");

    ref.altitude = XPLMFindDataRef("sim/flightmodel/misc/h_ind2");

    //request xpl config
    uint8_t v = idx_sim;
    server.write(&v, 1);

    /*ref.ail = XPLMFindDataRef("sim/joystick/yoke_roll_ratio");
  //ref.ail = XPLMFindDataRef("sim/flightmodel/controls/wing1l_ail1def");
  ref.elv= XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
  ref.rud = XPLMFindDataRef("sim/joystick/yoke_heading_ratio");
  //ref.thr = XPLMFindDataRef("sim/joystick/joystick_axis_values");
  ref.thr = XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro_use");
  ref.flap = XPLMFindDataRef("sim/flightmodel/controls/flaprqst");
  ref.brakes = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");

  ref.dsp = XPLMFindDataRef("sim/operation/override/override_planepath");
  ref.x = XPLMFindDataRef("sim/flightmodel/position/local_x");
  ref.y = XPLMFindDataRef("sim/flightmodel/position/local_y");
  ref.z = XPLMFindDataRef("sim/flightmodel/position/local_z");
*/
    XPLMRegisterFlightLoopCallback(flightLoopCallback, 1.0, nullptr);
    return 1;
}
//==============================================================================
float flightLoopCallback(float inElapsedSinceLastCall,
                         float inElapsedTimeSinceLastFlightLoop,
                         int inCounter,
                         void *inRefcon)
{
    if (!enabled)
        return 1.0;
    //send controls to sim
    uint rcnt = 0;
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
            stream.read(xpl);
            for (uint i = 0; i < PWM_CNT; ++i) {
                xbus::node::conf::ft_lstr_t &s = xpl[i];
                if (s[0] == 0)
                    continue;
                auto it1 = std::find(s.begin(), s.end(), '[');
                if (it1 != s.end()) {
                    *it1++ = 0;
                    auto it2 = std::find(it1, s.end(), ']');
                    if (it2 == s.end())
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
                XPLMDataRef ref = XPLMFindDataRef(s.data());
                if (!ref)
                    continue;
                xpl_type[i] = XPLMGetDataRefTypes(s.data());
                xpl_ref[i] = ref;
                printf("DataRef: %s\n", s.data());
            }
            break;
        case 1: //PWM outputs
            if (rcnt)
                break; //repeated packet, skip
            stream.read(pwm_ch);
            for (uint i = 0; i < PWM_CNT; i++) {
                if (!xpl_ref[i])
                    continue;
                //printf("set: %s=%f\n",(char*)xpl[i],(pwm_ch[i]));
                if (xpl_is_array[i]) {
                    XPLMSetDatavf(xpl_ref[i], &(pwm_ch[i]), xpl_array_idx[i], 1);
                    /*if(xpl_type[i]&xplmType_FloatArray) XPLMSetDatavf(xpl_ref[i],&(pwm_ch[i]),xpl_array_idx[i],1);
            else if(xpl_type[i]&xplmType_IntArray){
              int v=pwm_ch[i];
              XPLMSetDatavi(xpl_ref[i],&v,xpl_array_idx[i],1);
            }*/
                    continue;
                }
                XPLMSetDataf(xpl_ref[i], pwm_ch[i]);
                /*if(xpl_type[i]&xplmType_Float) XPLMSetDataf(xpl_ref[i],pwm_ch[i]);
          else if(xpl_type[i]&xplmType_Double) XPLMSetDatad(xpl_ref[i],pwm_ch[i]);
          else if(xpl_type[i]&xplmType_Int) XPLMSetDatai(xpl_ref[i],pwm_ch[i]);
          else XPLMSetDataf(xpl_ref[i],pwm_ch[i]);*/
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
    sendVars();
    return 0.1; //max freqs
}
//==============================================================================
_var_float noise(_var_float amp)
{
    _var_float v = amp * rand() / (_var_float) RAND_MAX - amp / 2;
    return v;
}
Vect noise_vect(_var_float amp)
{
    return Vect(noise(amp), noise(amp), noise(amp));
}
//==============================================================================
//==============================================================================
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
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 0);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 0);

    //server.close();

    enabled = false;
}
PLUGIN_API int XPluginEnable(void)
{
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_flightcontrol"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_roll"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_pitch"),1);
    //XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick_heading"),1);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_throttles"), 1);
    XPLMSetDatai(XPLMFindDataRef("sim/operation/override/override_joystick"), 1);

    enabled = true;
    return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam) {}
//==============================================================================
