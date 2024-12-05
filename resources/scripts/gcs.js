/**
 * APX Autopilot project <http://docs.uavos.com>
 * 
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 * 
 * This file is part of APX Ground Control.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted exclusively under the terms of the
 * LGPLv3 license. You should have received a copy of the license with
 * this file. If not, please visit: http://docs.uavos.com/gcs.
 */

// some helper functions also provided by tree objects

// basic helpers

function limit(v, min, max) {
    return v > max ? max : (v < min ? min : v);
}
limit.info = "limit value (v,min,max)"

function jhyst(v, h, max) {
    if (!max) max = 1
    v = limit(v, -max, max);
    if ((!h) || h === 0) return v
    if (Math.abs(v) < h) return 0
    if (v >= h) return (v - h) * (max / (max - h))
    if (v <= -h) return (v + h) * (max / (max - h))
}
jhyst.info = "joystick hysterezis (v,h)"

function trigger(v, a, b) {
    if (v == a) return b; else return a;
}
trigger.info = "trigger value of v to a or b";

function bound(v) {
    while (v >= 180) v -= 360;
    while (v < -180) v += 360;
    return v;
}
bound.info = "wrap angle -180..+180";


// system console
function req(n) {
    apx.fleet.current.mandala.fact(n).request();
}
req.info = "request var n from UAV";

function send(n) {
    apx.fleet.current.mandala.fact(n).send();
}
send.info = "send var n to UAV";

function serial(p, v) {
    apx.fleet.current.protocol.data.sendSerial(p, v);
}
serial.info = "send data v to serial port ID p";

function vmexec(f) {
    apx.fleet.current.requestScript(f);
}
vmexec.info = "execute function of onboard scripts";

function sleep(n) {
    application.engine.sleep(n);
}
sleep.info = "sleep n milliseconds";

function next() {
    apx.fleet.selectNext();
}
next.info = "switch to next unit";

function prev() {
    apx.fleet.selectPrev();
}
prev.info = "switch to previous unit";

// objects tree helper functions
function ls(a, b) {
    for (var i in a)
        if (typeof (a[i]) == b || !b)
            print(i + " - " + typeof (a[i]));
}
ls.info = "print members of type b for scope a";

function vars(a) {
    if (arguments.length == 0)
        a = this;
    for (var i in a)
        if (typeof (a[i]) == 'number')
            print(i + "=" + a[i]);
}
vars.info = "print variables for scope a";

function func(a) {
    if (arguments.length == 0)
        a = this;
    for (var i in a)
        if (typeof (a[i]) == 'function')
            print(i)
}
func.info = "print functions for scope a";

function help(name) {
    for (var i in this) {
        if (typeof (this[i]) != 'function')
            continue
        var f = this[i]
        if (!f.info)
            continue
        if (name && f != name)
            continue
        print(i + "\t" + f.info)
    }
}
help.info = "print help";


//predefined commands for variables
function hmsl() {
    est.ref.hmsl = est.pos.hmsl;
}
hmsl.info = "reset local GPS altitude";

function zrc() {
    cmd.rc.roll = 0;
    cmd.rc.pitch = 0;
    cmd.rc.thr = 0;
    cmd.rc.yaw = 0;
}
zrc.info = "reset pilot controls";

function flyTo(lat, lon) {
    apx.fleet.current.flyHere(apx.coordinate(lat, lon))
}
flyTo.info = "Set commanded position";



function inair(v) {
    cmd.ins.inair = v;
}
inair.info = "Set in-air status";

function sh(clist) {
    apx.fleet.current.nodes.shell(clist)
}
sh.info = "Node shell commands";





// Joystick movements
function jsw_R(v, h) { cmd.rc.roll = limit(v, -1, 1); }
function jsw_P(v, h) { cmd.rc.pitch = limit(v, -1, 1); }
function jsw_T(v) { cmd.rc.thr = limit(v, 0, 1); }
function jsw_Y(v, h) { cmd.rc.yaw = limit(v, -1, 1); }

// Encoders
function inc_ALT(v) { cmd.pos.altitude = limit(cmd.pos.altitude + v, 0, 30000); }
function inc_CRS(v) { cmd.pos.bearing = bound(cmd.pos.bearing + v); }
function inc_SPD(v) { cmd.pos.airspeed = limit(cmd.pos.airspeed + v, 0, 100); }
function inc_ADJ(v) { cmd.proc.adj = limit(cmd.proc.adj + v, -100, +100); }
function inc_FLAPS(v) { ctr.wing.flaps = limit(ctr.wing.flaps + v, 0, 1); }
function inc_AIRBRK(v) { ctr.wing.airbrk = limit(ctr.wing.airbrk + v, 0, 1); }


// Buttons
function btn_EMG() { cmd.proc.mode = proc_mode_EMG; }
function btn_RPV() { cmd.proc.mode = proc_mode_RPV; }
function btn_UAV() { cmd.proc.mode = proc_mode_UAV; }
function btn_WPT() { cmd.proc.mode = proc_mode_WPT; }
function btn_STBY() { cmd.proc.mode = proc_mode_STBY; }
function btn_TAXI() { cmd.proc.mode = proc_mode_TAXI; }
function btn_TAKEOFF() { cmd.proc.mode = proc_mode_TAKEOFF; }
function btn_LANDING() { cmd.proc.mode = proc_mode_LANDING; }

function btn_ESC() { cmd.proc.action = proc_action_reset; }
function btn_NEXT() { cmd.proc.action = proc_action_next; }

function btn_INC() { cmd.proc.action = proc_action_inc; }
function btn_DEC() { cmd.proc.action = proc_action_dec; }

function btn_ERS() { ctr.ers.launch = !ctr.ers.launch; }
function btn_REL() { ctr.ers.rel = !ctr.ers.rel; }
function btn_STRT() { ctr.eng.starter = !ctr.eng.starter; }
function set_STRT(v) { ctr.eng.starter = v; }


function btn_OVR() { cmd.eng.ovr = !cmd.eng.ovr; }
function btn_CUT() { cmd.eng.cut = !cmd.eng.cut; }
function btn_ENG() { ctr.pwr.eng = !ctr.pwr.eng; }

function btn_BRAKE() { ctr.str.brake = trigger(ctr.str.brake, 0, 1); }
function set_BRAKE(v) { ctr.str.brake = limit(v, 0, 1); }

function btn_FLAPS() { ctr.wing.flaps = trigger(ctr.wing.flaps, 0, 1); }
function btn_AIRBRK() { ctr.wing.airbrk = trigger(ctr.wing.airbrk, 0, 1); }

// Payload

function jsw_CR(v) { cmd.gimbal.roll = limit(v * 180, -180, 180); jsw_CY(v); }
function jsw_CP(v) { cmd.gimbal.pitch = limit(v * 180, -180, 180); }
function jsw_CY(v) { cmd.gimbal.yaw = limit(v * 180, -180, 180); }
function jsw_CT(v) { cmd.cam.zoom = limit(v, 0, 1); }

function btn_PLD() { ctr.pwr.payload = !ctr.pwr.payload; }

function btn_FM() { cmd.cam.fm = !cmd.cam.fm; }

function inc_BHDG(v) { cmd.gimbal.byaw = bound(cmd.gimbal.byaw + v * 0.01); }
function inc_BPITCH(v) { cmd.gimbal.bpitch = bound(cmd.gimbal.bpitch + v * 0.01); }

function inc_ZOOM(v) { cmd.cam.zoom = limit(cmd.cam.zoom + v * 0.01, 0, 1); }
function inc_FOCUS(v) { cmd.cam.focus = limit(cmd.cam.focus + v * 0.01, 0, 1); }
function btn_MAG() { cmd.cam.zoom = trigger(cmd.cam.zoom, 0, 1); }

function btn_SHOT() { cmd.cam.mode = cam_mode_single; }

function btn_DRP() { ctr.aux.drop = !ctr.aux.drop; }

function btn_CH1() { cmd.cam.ch = 0; }
function btn_CH2() { cmd.cam.ch = 1; }
function btn_CH3() { cmd.cam.ch = 2; }
