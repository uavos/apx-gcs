/*
 * Copyright (C) 2015 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * GCU QtScript functions
 * http://en.wikipedia.org/wiki/QtScript
 * QtScript is a scripting engine that has been part of the Qt cross-platform
 * application framework since version 4.3.0.
 * The scripting language is based on the ECMAScript standard with a few
 * extensions, such as QObject-style signal and slot connections.
 * Uses JavaScriptCore.
 *
 * This script is included in the context on gcu startup.
 * User functions can be defined in .gcu/scripts/gcu.js which is also included.
*/

function limit(v,min,max)
{
    return v>max?max:(v<min?min:v);
}
limit.info="limit value (v,min,max)"

function jhyst(v,h,max)
{
    if(!max)max=1
    v=limit(v,-max,max);
    if((!h) || h===0)return v
    if(Math.abs(v)<h)return 0
    if(v>=h) return (v-h)*(max/(max-h))
    if(v<=-h) return (v+h)*(max/(max-h))
}
jhyst.info="joystick hysterezis (v,h)"

// Joystick movements
function jsw_R(v,h){ rc_roll=limit(v,-1,1); }
function jsw_P(v,h){ rc_pitch=limit(v,-1,1); }
function jsw_T(v){ rc_throttle=limit(v,0,1); }
function jsw_Y(v,h){ rc_yaw=limit(v,-1,1); }

// Encoders
function inc_ALT(v){ cmd_altitude=limit(cmd_altitude+v,0,30000); }
function inc_CRS(v){ cmd_course=bound(cmd_course+v); }
function inc_SPD(v){ cmd_airspeed=limit(cmd_airspeed+v,0,100); }
function inc_ADJ(v){ rwAdj=limit(rwAdj+v,-100,+100); }
function inc_FLAPS(v){ ctr_flaps=limit(ctr_flaps+v,0,1); }
function inc_AIRBRK(v){ ctr_airbrk=limit(ctr_airbrk+v,0,1); }


// Buttons
function btn_EMG(){ mode=mode_EMG; }
function btn_RPV(){ mode=mode_RPV; }
function btn_UAV(){ mode=mode_UAV; }
function btn_WPT(){ mode=mode_WPT; }
function btn_HOME(){ mode=mode_HOME; }
function btn_STBY(){ mode=mode_STBY; }
function btn_TAXI(){ mode=mode_TAXI; }
function btn_TAKEOFF(){ mode=mode_TAKEOFF; }
function btn_LANDING(){ mode=mode_LANDING; }

function btn_ESC(){ stage=100; }
function btn_STAGE(){ stage++; }

function btn_INC(){ midx=midx_inc; }
function btn_DEC(){ midx=midx_dec; }

function btn_ERS(){ ctrb_ers=!ctrb_ers; }
function btn_REL(){ ctrb_rel=!ctrb_rel; }
function btn_STRT(){ ctrb_starter=!ctrb_starter; }
function set_STRT(v){ ctrb_starter=v; }


function btn_OVR(){ cmode_throvr=!cmode_throvr; }
function btn_CUT(){ cmode_thrcut=!cmode_thrcut; }
function btn_IGN(){ power_ignition=!power_ignition; }

function btn_BRAKE(){ ctr_brake=trigger(ctr_brake,0,1); }
function set_BRAKE(v){ ctr_brake=limit(v,0,1); }

function btn_FLAPS(){ ctr_flaps=trigger(ctr_flaps,0,1); }
function btn_AIRBRK(){ ctr_airbrk=trigger(ctr_airbrk,0,1); }

function btn_GEAR(){ ctrb_gear=!ctrb_gear; }

function btn_SW1(){ sw_sw1=!sw_sw1; }
function btn_SW2(){ sw_sw2=!sw_sw2; }

// Payload

function jsw_CR(v){ camcmd_roll=limit(v*180,-180,180); jsw_CY(v);}
function jsw_CP(v){ camcmd_pitch=limit(v*180,-180,180); }
function jsw_CY(v){ camcmd_yaw=limit(v*180,-180,180); }
function jsw_CT(v){ cam_zoom=limit(v,0,1); }


function btn_PLD(){ power_payload=!power_payload; }
function btn_FM(){ cam_opt_FM=!cam_opt_FM; }

function inc_BHDG(v){ cambias_yaw=bound(cambias_yaw+v*0.01); }
function inc_BPITCH(v){ cambias_pitch=bound(cambias_pitch+v*0.01); }
function inc_ZOOM(v){ cam_zoom=limit(cam_zoom+v*0.01,0,1); }
function inc_FOCUS(v){ cam_focus=limit(cam_focus+v*0.01,0,1); }

function btn_SHOT(){ cam_ctrb_shot=1; }
function btn_MAG(){ cam_zoom=trigger(cam_zoom,0,1); }

function btn_DRP(){ ctrb_drp=!ctrb_drp; }

function btn_CH1(){ cam_ch=0; }
function btn_CH2(){ cam_ch=1; }
