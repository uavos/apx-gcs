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

#include <MandalaMetaTree.h>
#include <map>

namespace mandala {

// Mandala fields mapping for old field names
// roll,pitch,yaw,Ax,Ay,Az,p,q,r,Hx,Hy,Hz,altitude,airspeed,vspeed,course,rpm,agl,slip,attack,venergy,ldratio,buoyancy,ctr_ailerons,ctr_elevator,ctr_throttle,ctr_rudder,ctr_collective,ctr_steering,ctr_airbrk,ctr_flaps,ctr_brake,ctr_mixture,ctr_engine,ctr_sweep,ctr_buoyancy,ctrb_ers,ctrb_rel,ctrb_drp,ctrb_pump,ctrb_starter,ctrb_horn,ctrb_rev,ctrb_gear,ctr_brakeL,ctr_brakeR,cmd_roll,cmd_pitch,cmd_yaw,cmd_north,cmd_east,cmd_course,cmd_rpm,cmd_altitude,cmd_airspeed,cmd_vspeed,cmd_slip,gps_lat,gps_lon,gps_hmsl,gps_Vnorth,gps_Veast,gps_Vdown,gps_time,fuel,frate,RSS,Ve,Vs,Vp,Vm,Ie,Is,Ip,Im,AT,RT,MT,ET,EGT,OT,OP,ilsb_armed,ilsb_approach,ilsb_offset,ilsb_platform,ils_IR,ils_RF,ils_HDG,ils_DME,ils_heading,ils_altitude,platform_lat,platform_lon,platform_hmsl,platform_Vnorth,platform_Veast,platform_Vdown,platform_hdg,range,radar_Vx,radar_Vy,radar_Vz,radar_dx,radar_dy,radar_dz,stage,mode,status_rc,status_gps,status_home,status_agl,status_modem,status_landed,status_touch,error_power,error_cas,error_pstatic,error_gyro,error_rpm,cmode_dlhd,cmode_thrcut,cmode_throvr,cmode_hover,cmode_hyaw,cmode_ahrs,cmode_nomag,power_ap,power_servo,power_ignition,power_payload,power_agl,power_xpdr,sw_starter,sw_lights,sw_taxi,sw_ice,sw_sw1,sw_sw2,sw_sw3,sw_sw4,sb_shutdown,sb_ers_err,sb_ers_disarm,sb_eng_err,sb_bat_err,sb_gen_err,wpidx,rwidx,twidx,piidx,midx,tgHDG,turnR,delta,loops,ETA,mtype,windSpd,windHdg,cas2tas,rwAdj,gps_SV,gps_SU,gps_jcw,gps_jstate,home_lat,home_lon,home_hmsl,altps_gnd,errcode,stab,cam_roll,cam_pitch,cam_yaw,turret_pitch,turret_heading,turret_armed,turret_shoot,turret_reload,turret_sw1,turret_sw2,turret_sw3,turret_sw4,userb_1,userb_2,userb_3,userb_4,userb_5,userb_6,userb_7,userb_8,user1,user2,user3,user4,user5,user6,ls_roll,ls_pitch,ls_cp,ls_spd,ls_ail,rs_roll,rs_pitch,rs_cp,rs_spd,rs_ail,vshape,cmd_vshape,lrs_croll,local,pos_north,pos_east,vel_north,vel_east,homeHDG,dHome,dWPT,Vx,Vy,dx,dy,gSpeed,wpHDG,rwDelta,rwDV,dl_period,dl_timestamp,altps,vario,vcas,denergy,rc_override,rc_roll,rc_pitch,rc_throttle,rc_yaw,cam_ch,cam_mode,camcmd_roll,camcmd_pitch,camcmd_yaw,cam_zoom,cam_focus,cambias_roll,cambias_pitch,cambias_yaw,cam_opt_PF,cam_opt_NIR,cam_opt_DSP,cam_opt_FMI,cam_opt_FM,cam_opt_laser,cam_src,cam_lat,cam_lon,cam_hmsl,camctr_roll,camctr_pitch,camctr_yaw,cams,cam_ctrb_shtr,cam_ctrb_arm,cam_ctrb_rec,cam_ctrb_zin,cam_ctrb_zout,cam_ctrb_aux,cam_tperiod,cam_timestamp,turretcmd_pitch,turretcmd_yaw,turretctr_roll,turretctr_pitch,turretctr_yaw,turret_mode,turretenc_pitch,turretenc_yaw,atscmd_pitch,atscmd_yaw,atsctr_pitch,atsctr_yaw,atsenc_pitch,atsenc_yaw,ats_mode,gcu_RSS,gcu_Ve,gcu_MT,VM1,VM2,VM3,VM4,VM5,VM6,VM7,VM8,VM9,VM10,VM11,VM12,VM13,VM14,VM15,VM16,VM17,VM18,VM19,VM20,VM21,VM22,VM23,VM24,VM25,VM26,VM27,VM28,VM29,VM30,VM31,VM32

static inline auto malias(const char *name, const char *mpath)
{
    return std::pair<std::string, std::string>(name, mpath);
}

const std::map<std::string, std::string> ALIAS_MAP = {
    malias("roll", "est.att.roll"),
    malias("pitch", "est.att.pitch"),
    malias("yaw", "est.att.yaw"),

    malias("Ax", "est.acc.x"),
    malias("Ay", "est.acc.y"),
    malias("Az", "est.acc.z"),

    malias("p", "est.gyro.x"),
    malias("q", "est.gyro.y"),
    malias("r", "est.gyro.z"),

    // malias("Hx","est.mag.x"),
    // malias("Hy","est.mag.y"),
    // malias("Hz","est.mag.z"),

    malias("altitude", "est.pos.altitude"),
    malias("airspeed", "est.air.airspeed"),
    malias("vspeed", "est.pos.vspeed"),
    malias("course", "est.pos.bearing"),
    malias("rpm", "est.eng.rpm"),
    malias("agl", "est.pos.agl"),
    malias("slip", "est.air.slip"),
    malias("attack", "est.air.aoa"),
    malias("venergy", "est.air.vse"),
    malias("ldratio", "est.air.ld"),
    malias("buoyancy", "sns.air.buo"),

    malias("ctr_ailerons", "ctr.att.ail"),
    malias("ctr_elevator", "ctr.att.elv"),
    malias("ctr_throttle", "ctr.eng.thr"),
    malias("ctr_rudder", "ctr.att.rud"),
    malias("ctr_collective", "ctr.eng.prop"),
    malias("ctr_steering", "ctr.str.rud"),
    malias("ctr_airbrk", "ctr.wing.airbrk"),
    malias("ctr_flaps", "ctr.wing.flaps"),
    malias("ctr_brake", "ctr.str.brake"),
    malias("ctr_mixture", "ctr.eng.choke"),
    malias("ctr_engine", "ctr.eng.tune"),
    malias("ctr_sweep", "ctr.wing.sweep"),
    malias("ctr_buoyancy", "ctr.wing.buo"),

    malias("ctrb_ers", "ctr.ers.launch"),
    malias("ctrb_rel", "ctr.ers.rel"),
    malias("ctrb_drp", "ctr.door.drop"),
    malias("ctrb_pump", "ctr.eng.fpump"),
    malias("ctrb_starter", "ctr.eng.starter"),
    malias("ctrb_horn", "ctr.aux.horn"),
    malias("ctrb_rev", "ctr.eng.tvec"),
    malias("ctrb_gear", "ctr.str.gear"),

    // malias<mandala::ctr::nav::str::>("ctr_brakeL"),
    // malias<mandala::ctr::nav::str::>("ctr_brakeR"),

    malias("cmd_roll", "cmd.att.roll"),
    malias("cmd_pitch", "cmd.att.pitch"),
    malias("cmd_yaw", "cmd.att.yaw"),

    // malias<mandala::cmd::nav::pos::>("cmd_north"),
    // malias<mandala::cmd::nav::pos::>("cmd_east"),

    malias("cmd_course", "cmd.pos.bearing"),
    malias("cmd_rpm", "cmd.eng.rpm"),
    malias("cmd_altitude", "cmd.pos.altitude"),
    malias("cmd_airspeed", "cmd.pos.airspeed"),
    malias("cmd_vspeed", "cmd.pos.vspeed"),
    malias("cmd_slip", "cmd.att.slip"),

    malias("gps_lat", "est.pos.lat"),
    malias("gps_lon", "est.pos.lon"),
    malias("gps_hmsl", "est.pos.hmsl"),

    // malias<mandala::est::nav::pos::>("gps_Vnorth"),
    // malias<mandala::est::nav::pos::>("gps_Veast"),
    malias("gps_Vdown", "est.lpos.vz"),

    malias("gps_time", "est.sys.time"),

    malias("fuel", "sns.fuel.level"),
    malias("frate", "sns.fuel.rate"),

    malias("RSS", "sns.com.rss"),
    malias("Ve", "sns.pwr.vsys"),
    malias("Vs", "sns.pwr.vsrv"),
    malias("Vp", "sns.pwr.vpld"),
    malias("Vm", "sns.eng.voltage"),
    malias("Ie", "sns.pwr.isys"),
    malias("Is", "sns.pwr.isrv"),
    malias("Ip", "sns.pwr.ipld"),
    malias("Im", "sns.eng.current"),

    malias("AT", "sns.air.temp"),
    malias("RT", "sns.aux.rt"),
    malias("MT", "sns.com.temp"),
    malias("ET", "sns.eng.temp"),
    malias("EGT", "sns.eng.egt"),
    malias("OT", "sns.eng.ot"),
    malias("OP", "sns.eng.op"),

    // malias("ilsb_armed","sns.ils.armed"),
    // malias("ilsb_approach","sns.ils.approach"),
    // malias("ilsb_offset","sns.ils.offset"),
    // malias("ilsb_platform","sns.ils.platform"),
    // malias("ils_IR","sns.ils.ir"),
    // malias("ils_RF","sns.ils.rf"),
    // malias("ils_HDG","sns.ils.hdg"),
    // malias("ils_DME","sns.ils.dme"),
    // malias("ils_heading","sns.ils.heading"),
    // malias("ils_altitude","sns.ils.altitude"),

    malias("platform_lat", "sns.pfm.lat"),
    malias("platform_lon", "sns.pfm.lon"),
    malias("platform_hmsl", "sns.pfm.hmsl"),
    malias("platform_Vnorth", "sns.pfm.vn"),
    malias("platform_Veast", "sns.pfm.ve"),
    malias("platform_Vdown", "sns.pfm.vd"),
    malias("platform_hdg", "sns.pfm.yaw"),

    malias("range", "sns.tcas.dist"),
    malias("radar_Vx", "sns.vps.vx"),
    malias("radar_Vy", "sns.vps.vy"),
    malias("radar_Vz", "sns.vps.vz"),
    malias("radar_dx", "sns.vps.x"),
    malias("radar_dy", "sns.vps.y"),
    malias("radar_dz", "sns.vps.z"),

    malias("stage", "cmd.proc.stage"),
    malias("mode", "cmd.proc.mode"),

    // malias("status_rc","cmd.rc.mode"),
    malias("status_gps", "est.pos.valid"),
    malias("status_home", "est.ref.status"),
    malias("status_agl", "sns.agl.status"),
    malias("status_modem", "sns.com.status"),

    // malias("status_landed","est.ins.rest"),
    // malias("status_touch","sns.aux.fgear"),

    // malias("error_power","sns.pwr.error"),
    // malias("error_cas","sns.com.error"),
    // malias("error_pstatic","sns.pwr.error"),
    // malias("error_gyro","sns.gyro.error"),
    // malias("error_rpm","sns.eng.error"),

    // malias("cmode_dlhd","cmd.proc.mode"),
    malias("cmode_thrcut", "cmd.eng.cut"),
    malias("cmode_throvr", "cmd.eng.ovr"),
    // malias("cmode_hover","cmd.proc.hover"),
    // malias("cmode_hyaw","cmd.proc.yaw"),
    malias("cmode_ahrs", "cmd.ins.nogps"),
    malias("cmode_nomag", "cmd.ins.nomag"),

    malias("power_ap", "ctr.pwr.ap"),
    malias("power_servo", "ctr.pwr.servo"),
    malias("power_ignition", "ctr.pwr.eng"),
    malias("power_payload", "ctr.pwr.payload"),
    malias("power_agl", "ctr.pwr.agl"),
    malias("power_xpdr", "ctr.pwr.xpdr"),

    malias("sw_starter", "ctr.eng.starter"),
    malias("sw_lights", "ctr.light.nav"),
    malias("sw_taxi", "ctr.light.taxi"),
    malias("sw_ice", "ctr.pwr.ice"),
    malias("sw_sw1", "ctr.sw.sw1"),
    malias("sw_sw2", "ctr.sw.sw2"),
    malias("sw_sw3", "ctr.sw.sw3"),
    malias("sw_sw4", "ctr.sw.sw4"),

    // malias("sb_shutdown","ctr.sw.shutdown"),
    // malias("sb_ers_err","ctr.ers.error"),
    // malias("sb_ers_disarm","ctr.ers.disarm"),
    // malias("sb_eng_err","ctr.eng.error"),
    // malias("sb_bat_err","ctr.pwr.error"),
    // malias("sb_gen_err","ctr.pwr.error"),

    malias("wpidx", "cmd.proc.wp"),
    malias("rwidx", "cmd.proc.rw"),
    // malias("twidx","cmd.proc.tw"),
    malias("piidx", "cmd.proc.pi"),
    // malias("midx","cmd.proc.mi"),

    malias("tgHDG", "est.wpt.thdg"),
    malias("turnR", "cmd.pos.radius"),
    malias("delta", "est.wpt.derr"),
    malias("loops", "cmd.proc.orbs"),
    malias("ETA", "est.wpt.eta"),
    // malias("mtype","cmd.proc.mtype"),

    malias("windSpd", "est.wind.speed"),
    malias("windHdg", "est.wind.heading"),

    malias("cas2tas", "est.air.ktas"),
    malias("rwAdj", "cmd.proc.adj"),

    malias("gps_SV", "sns.gps.sv"),
    malias("gps_SU", "sns.gps.su"),
    // malias("gps_jcw","sns.gps.jcw"),
    // malias("gps_jstate","sns.gps.jstate"),

    malias("home_lat", "est.ref.lat"),
    malias("home_lon", "est.ref.lon"),
    malias("home_hmsl", "est.ref.hmsl"),

    // malias("altps_gnd","est.ref.altps"),
    // malias("errcode","est.ref.err"),

    malias("stab", "est.air.stab"),

    // malias("cam_roll","est.cam.roll"),
    // malias("cam_pitch","est.cam.pitch"),
    // malias("cam_yaw","est.cam.yaw"),

    // malias("turret_pitch","sns.turret.pitch"),
    // malias("turret_heading","sns.turret.yaw"),
    // malias("turret_armed","est.turret.armed"),
    // malias("turret_shoot","est.turret.shoot"),
    // malias("turret_reload","est.turret.reload"),

    malias("turret_sw1", "ctr.sw.sw5"),
    malias("turret_sw2", "ctr.sw.sw6"),
    malias("turret_sw3", "ctr.sw.sw7"),
    malias("turret_sw4", "ctr.sw.sw8"),

    malias("userb_1", "est.usrb.b1"),
    malias("userb_2", "est.usrb.b2"),
    malias("userb_3", "est.usrb.b3"),
    malias("userb_4", "est.usrb.b4"),
    malias("userb_5", "est.usrb.b5"),
    malias("userb_6", "est.usrb.b6"),
    malias("userb_7", "est.usrb.b7"),
    malias("userb_8", "est.usrb.b8"),

    malias("user1", "est.usr.u1"),
    malias("user2", "est.usr.u2"),
    malias("user3", "est.usr.u3"),
    malias("user4", "est.usr.u4"),
    malias("user5", "est.usr.u5"),
    malias("user6", "est.usr.u6"),

    malias("ls_roll", "est.haps.roll1"),
    malias("ls_pitch", "est.haps.pitch1"),
    malias("ls_cp", "cmd.haps.cpitch1"),
    malias("ls_spd", "est.haps.spd1"),
    malias("ls_ail", "ctr.haps.ail1"),

    malias("rs_roll", "est.haps.roll2"),
    malias("rs_pitch", "est.haps.pitch2"),
    malias("rs_cp", "cmd.haps.cpitch2"),
    malias("rs_spd", "est.haps.spd2"),
    malias("rs_ail", "ctr.haps.ail2"),

    malias("vshape", "est.haps.shape"),
    malias("cmd_vshape", "cmd.haps.cshape"),
    malias("lrs_croll", "est.haps.roll"),

    // malias<mandala::est::nav::local::pos::north>("local"),

    malias("pos_north", "est.lpos.n"),
    malias("pos_east", "est.lpos.e"),

    // malias("vel_north","est.lvel.n"),
    // malias("vel_east","est.lvel.e"),

    // malias("homeHDG","est.ref.hdg"),
    // malias("dHome","est.ref.dHome"),
    malias("dWPT", "est.wpt.dist"),

    malias("Vx", "est.lpos.vx"),
    malias("Vy", "est.lpos.vy"),

    // malias("dx","est.lpos.dx"),
    // malias("dy","est.lpos.dy"),

    malias("gSpeed", "est.pos.speed"),
    malias("wpHDG", "est.wpt.hdg"),
    malias("rwDelta", "est.wpt.xtrack"),
    // malias("rwDV","est.wpt.dv"),

    // malias("dl_period","cmd.proc.dl_period"),
    // malias("dl_timestamp","cmd.proc.dl_timestamp"),

    // malias("altps","est.pos.altps"),
    // malias("vario","est.pos.vario"),
    // malias("vcas","est.pos.vcas"),
    // malias("denergy","est.pos.denergy"),

    malias("rc_override", "cmd.rc.mode"),
    malias("rc_roll", "cmd.rc.roll"),
    malias("rc_pitch", "cmd.rc.pitch"),
    malias("rc_throttle", "cmd.rc.thr"),
    malias("rc_yaw", "cmd.rc.yaw"),

    // all other vars are ignored

    // malias("cam_ch","est.cam.ch"),
    // malias("cam_mode","est.cam.mode"),
    // malias("camcmd_roll","cmd.cam.roll"),
    // malias("camcmd_pitch","cmd.cam.pitch"),
    // malias("camcmd_yaw","cmd.cam.yaw"),
    // malias("cam_zoom","cmd.cam.zoom"),
    // malias("cam_focus","cmd.cam.focus"),
    // malias<mandala::cmd::nav::cam::bias::roll>("cambias_roll"),
    // malias<mandala::cmd::nav::cam::bias::pitch>("cambias_pitch"),
    // malias<mandala::cmd::nav::cam::bias::yaw>("cambias_yaw"),
    // malias<mandala::cmd::nav::cam::opt::pf>("cam_opt_PF"),
    // malias<mandala::cmd::nav::cam::opt::nir>("cam_opt_NIR"),
    // malias<mandala::cmd::nav::cam::opt::dsp>("cam_opt_DSP"),
    // malias<mandala::cmd::nav::cam::opt::fmi>("cam_opt_FMI"),
    // malias<mandala::cmd::nav::cam::opt::fm>("cam_opt_FM"),
    // malias<mandala::cmd::nav::cam::opt::laser>("cam_opt_laser"),
    // malias("cam_src","cmd.cam.src"),
    // malias("cam_lat","cmd.cam.lat"),
    // malias("cam_lon","cmd.cam.lon"),
    // malias("cam_hmsl","cmd.cam.hmsl"),
    // malias<mandala::cmd::nav::cam::ctr::roll>("camctr_roll"),
    // malias<mandala::cmd::nav::cam::ctr::pitch>("camctr_pitch"),
    // malias<mandala::cmd::nav::cam::ctr::yaw>("camctr_yaw"),
    // malias("cams","cmd.cam.s"),
    // malias<mandala::cmd::nav::cam::ctr::shtr>("cam_ctrb_shtr"),
    // malias<mandala::cmd::nav::cam::ctr::arm>("cam_ctrb_arm"),
    // malias<mandala::cmd::nav::cam::ctr::rec>("cam_ctrb_rec"),
    // malias<mandala::cmd::nav::cam::ctr::zin>("cam_ctrb_zin"),
    // malias<mandala::cmd::nav::cam::ctr::zout>("cam_ctrb_zout"),
    // malias<mandala::cmd::nav::cam::ctr::aux>("cam_ctrb_aux"),
    // malias("cam_tperiod","cmd.cam.tperiod"),
    // malias("cam_timestamp","cmd.cam.timestamp"),

    // malias<mandala::est::nav::turret::cmd::pitch>("turretcmd_pitch"),
    // malias<mandala::est::nav::turret::cmd::yaw>("turretcmd_yaw"),
    // malias<mandala::est::nav::turret::ctr::roll>("turretctr_roll"),
    // malias<mandala::est::nav::turret::ctr::pitch>("turretctr_pitch"),
    // malias<mandala::est::nav::turret::ctr::yaw>("turretctr_yaw"),
    // malias("turret_mode","est.turret.mode"),
    // malias<mandala::est::nav::turret::enc::pitch>("turretenc_pitch"),
    // malias<mandala::est::nav::turret::enc::yaw>("turretenc_yaw"),

    // malias<mandala::est::nav::ats::cmd::pitch>("atscmd_pitch"),
    // malias<mandala::est::nav::ats::cmd::yaw>("atscmd_yaw"),
    // malias<mandala::est::nav::ats::ctr::pitch>("atsctr_pitch"),
    // malias<mandala::est::nav::ats::ctr::yaw>("atsctr_yaw"),
    // malias<mandala::est::nav::ats::enc::pitch>("atsenc_pitch"),
    // malias<mandala::est::nav::ats::enc::yaw>("atsenc_yaw"),
    // malias("ats_mode","est.ats.mode"),

    // malias("gcu_RSS","sns.gcu.rss"),
    // malias("gcu_Ve","sns.gcu.ve"),
    // malias("gcu_MT","sns.gcu.mt"),

    malias("VM1", "est.usrf.f1"),
    malias("VM2", "est.usrf.f2"),
    malias("VM3", "est.usrf.f3"),
    malias("VM4", "est.usrf.f4"),
    malias("VM5", "est.usrf.f5"),
    malias("VM6", "est.usrf.f6"),
    malias("VM7", "est.usrf.f7"),
    malias("VM8", "est.usrf.f8"),
    malias("VM9", "est.usrf.f9"),
    malias("VM10", "est.usrf.f10"),
    malias("VM11", "est.usrf.f11"),
    malias("VM12", "est.usrf.f12"),
    malias("VM13", "est.usrf.f13"),
    malias("VM14", "est.usrf.f14"),
    malias("VM15", "est.usrf.f15"),

};

} // namespace mandala
