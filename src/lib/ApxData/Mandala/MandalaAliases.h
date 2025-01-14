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

template<typename M>
constexpr auto malias(const char *name)
{
    return std::pair<std::string, mandala::uid_t>(name, M::uid);
}

const std::map<std::string, mandala::uid_t> ALIAS_MAP = {
    malias<mandala::est::nav::att::roll>("roll"),
    malias<mandala::est::nav::att::pitch>("pitch"),
    malias<mandala::est::nav::att::yaw>("yaw"),

    malias<mandala::est::nav::acc::x>("Ax"),
    malias<mandala::est::nav::acc::y>("Ay"),
    malias<mandala::est::nav::acc::z>("Az"),

    malias<mandala::est::nav::gyro::x>("p"),
    malias<mandala::est::nav::gyro::y>("q"),
    malias<mandala::est::nav::gyro::z>("r"),

    // malias<mandala::est::nav::mag::x>("Hx"),
    // malias<mandala::est::nav::mag::y>("Hy"),
    // malias<mandala::est::nav::mag::z>("Hz"),

    malias<mandala::est::nav::pos::altitude>("altitude"),
    malias<mandala::est::nav::air::airspeed>("airspeed"),
    malias<mandala::est::nav::pos::vspeed>("vspeed"),
    malias<mandala::est::nav::pos::bearing>("course"),
    malias<mandala::est::nav::eng::rpm>("rpm"),
    malias<mandala::est::nav::pos::agl>("agl"),
    malias<mandala::est::nav::air::slip>("slip"),
    malias<mandala::est::nav::air::aoa>("attack"),
    malias<mandala::est::nav::air::vse>("venergy"),
    malias<mandala::est::nav::air::ld>("ldratio"),
    malias<mandala::sns::nav::air::buo>("buoyancy"),

    malias<mandala::ctr::nav::att::ail>("ctr_ailerons"),
    malias<mandala::ctr::nav::att::elv>("ctr_elevator"),
    malias<mandala::ctr::nav::eng::thr>("ctr_throttle"),
    malias<mandala::ctr::nav::att::rud>("ctr_rudder"),
    malias<mandala::ctr::nav::eng::prop>("ctr_collective"),
    malias<mandala::ctr::nav::str::rud>("ctr_steering"),
    malias<mandala::ctr::nav::wing::airbrk>("ctr_airbrk"),
    malias<mandala::ctr::nav::wing::flaps>("ctr_flaps"),
    malias<mandala::ctr::nav::str::brake>("ctr_brake"),
    malias<mandala::ctr::nav::eng::choke>("ctr_mixture"),
    malias<mandala::ctr::nav::eng::tune>("ctr_engine"),
    malias<mandala::ctr::nav::wing::sweep>("ctr_sweep"),
    malias<mandala::ctr::nav::wing::buo>("ctr_buoyancy"),

    malias<mandala::ctr::env::ers::launch>("ctrb_ers"),
    malias<mandala::ctr::env::ers::rel>("ctrb_rel"),
    malias<mandala::ctr::env::door::drop>("ctrb_drp"),
    malias<mandala::ctr::nav::eng::fpump>("ctrb_pump"),
    malias<mandala::ctr::nav::eng::starter>("ctrb_starter"),
    malias<mandala::ctr::env::aux::horn>("ctrb_horn"),
    malias<mandala::ctr::nav::eng::tvec>("ctrb_rev"),
    malias<mandala::ctr::nav::str::gear>("ctrb_gear"),

    // malias<mandala::ctr::nav::str::>("ctr_brakeL"),
    // malias<mandala::ctr::nav::str::>("ctr_brakeR"),

    malias<mandala::cmd::nav::att::roll>("cmd_roll"),
    malias<mandala::cmd::nav::att::pitch>("cmd_pitch"),
    malias<mandala::cmd::nav::att::yaw>("cmd_yaw"),

    // malias<mandala::cmd::nav::pos::>("cmd_north"),
    // malias<mandala::cmd::nav::pos::>("cmd_east"),

    malias<mandala::cmd::nav::pos::bearing>("cmd_course"),
    malias<mandala::cmd::nav::eng::rpm>("cmd_rpm"),
    malias<mandala::cmd::nav::pos::altitude>("cmd_altitude"),
    malias<mandala::cmd::nav::pos::airspeed>("cmd_airspeed"),
    malias<mandala::cmd::nav::pos::vspeed>("cmd_vspeed"),
    malias<mandala::cmd::nav::att::slip>("cmd_slip"),

    malias<mandala::est::nav::pos::lat>("gps_lat"),
    malias<mandala::est::nav::pos::lon>("gps_lon"),
    malias<mandala::est::nav::pos::hmsl>("gps_hmsl"),

    // malias<mandala::est::nav::pos::>("gps_Vnorth"),
    // malias<mandala::est::nav::pos::>("gps_Veast"),
    malias<mandala::est::nav::lpos::vz>("gps_Vdown"),

    malias<mandala::est::env::sys::time>("gps_time"),

    malias<mandala::sns::env::fuel::level>("fuel"),
    malias<mandala::sns::env::fuel::rate>("frate"),

    malias<mandala::sns::env::com::rss>("RSS"),
    malias<mandala::sns::env::pwr::vsys>("Ve"),
    malias<mandala::sns::env::pwr::vsrv>("Vs"),
    malias<mandala::sns::env::pwr::vpld>("Vp"),
    malias<mandala::sns::env::eng::voltage>("Vm"),
    malias<mandala::sns::env::pwr::isys>("Ie"),
    malias<mandala::sns::env::pwr::isrv>("Is"),
    malias<mandala::sns::env::pwr::ipld>("Ip"),
    malias<mandala::sns::env::eng::current>("Im"),

    malias<mandala::sns::nav::air::temp>("AT"),
    malias<mandala::sns::env::aux::rt>("RT"),
    malias<mandala::sns::env::com::temp>("MT"),
    malias<mandala::sns::env::eng::temp>("ET"),
    malias<mandala::sns::env::eng::egt>("EGT"),
    malias<mandala::sns::env::eng::ot>("OT"),
    malias<mandala::sns::env::eng::op>("OP"),

    // malias<mandala::sns::nav::ils::armed>("ilsb_armed"),
    // malias<mandala::sns::nav::ils::approach>("ilsb_approach"),
    // malias<mandala::sns::nav::ils::offset>("ilsb_offset"),
    // malias<mandala::sns::nav::ils::platform>("ilsb_platform"),
    // malias<mandala::sns::nav::ils::ir>("ils_IR"),
    // malias<mandala::sns::nav::ils::rf>("ils_RF"),
    // malias<mandala::sns::nav::ils::hdg>("ils_HDG"),
    // malias<mandala::sns::nav::ils::dme>("ils_DME"),
    // malias<mandala::sns::nav::ils::heading>("ils_heading"),
    // malias<mandala::sns::nav::ils::altitude>("ils_altitude"),

    malias<mandala::sns::nav::pfm::lat>("platform_lat"),
    malias<mandala::sns::nav::pfm::lon>("platform_lon"),
    malias<mandala::sns::nav::pfm::hmsl>("platform_hmsl"),
    malias<mandala::sns::nav::pfm::vn>("platform_Vnorth"),
    malias<mandala::sns::nav::pfm::ve>("platform_Veast"),
    malias<mandala::sns::nav::pfm::vd>("platform_Vdown"),
    malias<mandala::sns::nav::pfm::yaw>("platform_hdg"),

    malias<mandala::sns::nav::tcas::dist>("range"),
    malias<mandala::sns::nav::vps::vx>("radar_Vx"),
    malias<mandala::sns::nav::vps::vy>("radar_Vy"),
    malias<mandala::sns::nav::vps::vz>("radar_Vz"),
    malias<mandala::sns::nav::vps::x>("radar_dx"),
    malias<mandala::sns::nav::vps::y>("radar_dy"),
    malias<mandala::sns::nav::vps::z>("radar_dz"),

    malias<mandala::cmd::nav::proc::stage>("stage"),
    malias<mandala::cmd::nav::proc::mode>("mode"),

    // malias<mandala::cmd::nav::rc::mode>("status_rc"),
    malias<mandala::est::nav::pos::valid>("status_gps"),
    malias<mandala::est::nav::ref::status>("status_home"),
    malias<mandala::sns::nav::agl::status>("status_agl"),
    malias<mandala::sns::env::com::status>("status_modem"),

    // malias<mandala::est::nav::ins::rest>("status_landed"),
    // malias<mandala::sns::env::aux::fgear>("status_touch"),

    // malias<mandala::sns::env::pwr::error>("error_power"),
    // malias<mandala::sns::env::com::error>("error_cas"),
    // malias<mandala::sns::env::pwr::error>("error_pstatic"),
    // malias<mandala::sns::env::gyro::error>("error_gyro"),
    // malias<mandala::sns::env::eng::error>("error_rpm"),

    // malias<mandala::cmd::nav::proc::mode>("cmode_dlhd"),
    malias<mandala::cmd::nav::eng::cut>("cmode_thrcut"),
    malias<mandala::cmd::nav::eng::ovr>("cmode_throvr"),
    // malias<mandala::cmd::nav::proc::hover>("cmode_hover"),
    // malias<mandala::cmd::nav::proc::yaw>("cmode_hyaw"),
    malias<mandala::cmd::nav::ins::nogps>("cmode_ahrs"),
    malias<mandala::cmd::nav::ins::nomag>("cmode_nomag"),

    malias<mandala::ctr::env::pwr::ap>("power_ap"),
    malias<mandala::ctr::env::pwr::servo>("power_servo"),
    malias<mandala::ctr::env::pwr::eng>("power_ignition"),
    malias<mandala::ctr::env::pwr::payload>("power_payload"),
    malias<mandala::ctr::env::pwr::agl>("power_agl"),
    malias<mandala::ctr::env::pwr::xpdr>("power_xpdr"),

    malias<mandala::ctr::nav::eng::starter>("sw_starter"),
    malias<mandala::ctr::env::light::nav>("sw_lights"),
    malias<mandala::ctr::env::light::taxi>("sw_taxi"),
    malias<mandala::ctr::env::pwr::ice>("sw_ice"),
    malias<mandala::ctr::env::sw::sw1>("sw_sw1"),
    malias<mandala::ctr::env::sw::sw2>("sw_sw2"),
    malias<mandala::ctr::env::sw::sw3>("sw_sw3"),
    malias<mandala::ctr::env::sw::sw4>("sw_sw4"),

    // malias<mandala::ctr::env::sw::shutdown>("sb_shutdown"),
    // malias<mandala::ctr::env::ers::error>("sb_ers_err"),
    // malias<mandala::ctr::env::ers::disarm>("sb_ers_disarm"),
    // malias<mandala::ctr::env::eng::error>("sb_eng_err"),
    // malias<mandala::ctr::env::pwr::error>("sb_bat_err"),
    // malias<mandala::ctr::env::pwr::error>("sb_gen_err"),

    malias<mandala::cmd::nav::proc::wp>("wpidx"),
    malias<mandala::cmd::nav::proc::rw>("rwidx"),
    // malias<mandala::cmd::nav::proc::tw>("twidx"),
    malias<mandala::cmd::nav::proc::pi>("piidx"),
    // malias<mandala::cmd::nav::proc::mi>("midx"),

    malias<mandala::est::nav::wpt::thdg>("tgHDG"),
    malias<mandala::cmd::nav::pos::radius>("turnR"),
    malias<mandala::est::nav::wpt::derr>("delta"),
    malias<mandala::cmd::nav::proc::orbs>("loops"),
    malias<mandala::est::nav::wpt::eta>("ETA"),
    // malias<mandala::cmd::nav::proc::mtype>("mtype"),

    malias<mandala::est::nav::wind::speed>("windSpd"),
    malias<mandala::est::nav::wind::heading>("windHdg"),

    malias<mandala::est::nav::air::ktas>("cas2tas"),
    malias<mandala::cmd::nav::proc::adj>("rwAdj"),

    malias<mandala::sns::nav::gps::sv>("gps_SV"),
    malias<mandala::sns::nav::gps::su>("gps_SU"),
    // malias<mandala::sns::nav::gps::jcw>("gps_jcw"),
    // malias<mandala::sns::nav::gps::jstate>("gps_jstate"),

    malias<mandala::est::nav::ref::lat>("home_lat"),
    malias<mandala::est::nav::ref::lon>("home_lon"),
    malias<mandala::est::nav::ref::hmsl>("home_hmsl"),

    // malias<mandala::est::nav::ref::altps>("altps_gnd"),
    // malias<mandala::est::nav::ref::err>("errcode"),

    malias<mandala::est::nav::air::stab>("stab"),

    // malias<mandala::est::nav::cam::roll>("cam_roll"),
    // malias<mandala::est::nav::cam::pitch>("cam_pitch"),
    // malias<mandala::est::nav::cam::yaw>("cam_yaw"),

    // malias<mandala::sns::env::turret::pitch>("turret_pitch"),
    // malias<mandala::sns::env::turret::yaw>("turret_heading"),
    // malias<mandala::est::nav::turret::armed>("turret_armed"),
    // malias<mandala::est::nav::turret::shoot>("turret_shoot"),
    // malias<mandala::est::nav::turret::reload>("turret_reload"),

    malias<mandala::ctr::env::sw::sw5>("turret_sw1"),
    malias<mandala::ctr::env::sw::sw6>("turret_sw2"),
    malias<mandala::ctr::env::sw::sw7>("turret_sw3"),
    malias<mandala::ctr::env::sw::sw8>("turret_sw4"),

    malias<mandala::est::env::usrb::b1>("userb_1"),
    malias<mandala::est::env::usrb::b2>("userb_2"),
    malias<mandala::est::env::usrb::b3>("userb_3"),
    malias<mandala::est::env::usrb::b4>("userb_4"),
    malias<mandala::est::env::usrb::b5>("userb_5"),
    malias<mandala::est::env::usrb::b6>("userb_6"),
    malias<mandala::est::env::usrb::b7>("userb_7"),
    malias<mandala::est::env::usrb::b8>("userb_8"),

    malias<mandala::est::env::usr::u1>("user1"),
    malias<mandala::est::env::usr::u2>("user2"),
    malias<mandala::est::env::usr::u3>("user3"),
    malias<mandala::est::env::usr::u4>("user4"),
    malias<mandala::est::env::usr::u5>("user5"),
    malias<mandala::est::env::usr::u6>("user6"),

    malias<mandala::est::env::haps::roll1>("ls_roll"),
    malias<mandala::est::env::haps::pitch1>("ls_pitch"),
    malias<mandala::est::env::haps::cpitch1>("ls_cp"),
    malias<mandala::est::env::haps::spd1>("ls_spd"),
    malias<mandala::est::env::haps::ail1>("ls_ail"),

    malias<mandala::est::env::haps::roll2>("rs_roll"),
    malias<mandala::est::env::haps::pitch2>("rs_pitch"),
    malias<mandala::est::env::haps::cpitch2>("rs_cp"),
    malias<mandala::est::env::haps::spd2>("rs_spd"),
    malias<mandala::est::env::haps::ail2>("rs_ail"),

    malias<mandala::est::env::haps::shape>("vshape"),
    malias<mandala::est::env::haps::cshape>("cmd_vshape"),
    malias<mandala::est::env::haps::roll>("lrs_croll"),

    // malias<mandala::est::nav::local::pos::north>("local"),

    malias<mandala::est::nav::lpos::n>("pos_north"),
    malias<mandala::est::nav::lpos::e>("pos_east"),

    // malias<mandala::est::nav::lvel::n>("vel_north"),
    // malias<mandala::est::nav::lvel::e>("vel_east"),

    // malias<mandala::est::nav::ref::hdg>("homeHDG"),
    // malias<mandala::est::nav::ref::dHome>("dHome"),
    malias<mandala::est::nav::wpt::dist>("dWPT"),

    malias<mandala::est::nav::lpos::vx>("Vx"),
    malias<mandala::est::nav::lpos::vy>("Vy"),

    // malias<mandala::est::nav::lpos::dx>("dx"),
    // malias<mandala::est::nav::lpos::dy>("dy"),

    malias<mandala::est::nav::pos::speed>("gSpeed"),
    malias<mandala::est::nav::wpt::hdg>("wpHDG"),
    malias<mandala::est::nav::wpt::xtrack>("rwDelta"),
    // malias<mandala::est::nav::wpt::dv>("rwDV"),

    // malias<mandala::cmd::nav::proc::dl_period>("dl_period"),
    // malias<mandala::cmd::nav::proc::dl_timestamp>("dl_timestamp"),

    // malias<mandala::est::nav::pos::altps>("altps"),
    // malias<mandala::est::nav::pos::vario>("vario"),
    // malias<mandala::est::nav::pos::vcas>("vcas"),
    // malias<mandala::est::nav::pos::denergy>("denergy"),

    malias<mandala::cmd::nav::rc::mode>("rc_override"),
    malias<mandala::cmd::nav::rc::roll>("rc_roll"),
    malias<mandala::cmd::nav::rc::pitch>("rc_pitch"),
    malias<mandala::cmd::nav::rc::thr>("rc_throttle"),
    malias<mandala::cmd::nav::rc::yaw>("rc_yaw"),

    // all other vars are ignored

    // malias<mandala::est::nav::cam::ch>("cam_ch"),
    // malias<mandala::est::nav::cam::mode>("cam_mode"),
    // malias<mandala::cmd::nav::cam::roll>("camcmd_roll"),
    // malias<mandala::cmd::nav::cam::pitch>("camcmd_pitch"),
    // malias<mandala::cmd::nav::cam::yaw>("camcmd_yaw"),
    // malias<mandala::cmd::nav::cam::zoom>("cam_zoom"),
    // malias<mandala::cmd::nav::cam::focus>("cam_focus"),
    // malias<mandala::cmd::nav::cam::bias::roll>("cambias_roll"),
    // malias<mandala::cmd::nav::cam::bias::pitch>("cambias_pitch"),
    // malias<mandala::cmd::nav::cam::bias::yaw>("cambias_yaw"),
    // malias<mandala::cmd::nav::cam::opt::pf>("cam_opt_PF"),
    // malias<mandala::cmd::nav::cam::opt::nir>("cam_opt_NIR"),
    // malias<mandala::cmd::nav::cam::opt::dsp>("cam_opt_DSP"),
    // malias<mandala::cmd::nav::cam::opt::fmi>("cam_opt_FMI"),
    // malias<mandala::cmd::nav::cam::opt::fm>("cam_opt_FM"),
    // malias<mandala::cmd::nav::cam::opt::laser>("cam_opt_laser"),
    // malias<mandala::cmd::nav::cam::src>("cam_src"),
    // malias<mandala::cmd::nav::cam::lat>("cam_lat"),
    // malias<mandala::cmd::nav::cam::lon>("cam_lon"),
    // malias<mandala::cmd::nav::cam::hmsl>("cam_hmsl"),
    // malias<mandala::cmd::nav::cam::ctr::roll>("camctr_roll"),
    // malias<mandala::cmd::nav::cam::ctr::pitch>("camctr_pitch"),
    // malias<mandala::cmd::nav::cam::ctr::yaw>("camctr_yaw"),
    // malias<mandala::cmd::nav::cam::s>("cams"),
    // malias<mandala::cmd::nav::cam::ctr::shtr>("cam_ctrb_shtr"),
    // malias<mandala::cmd::nav::cam::ctr::arm>("cam_ctrb_arm"),
    // malias<mandala::cmd::nav::cam::ctr::rec>("cam_ctrb_rec"),
    // malias<mandala::cmd::nav::cam::ctr::zin>("cam_ctrb_zin"),
    // malias<mandala::cmd::nav::cam::ctr::zout>("cam_ctrb_zout"),
    // malias<mandala::cmd::nav::cam::ctr::aux>("cam_ctrb_aux"),
    // malias<mandala::cmd::nav::cam::tperiod>("cam_tperiod"),
    // malias<mandala::cmd::nav::cam::timestamp>("cam_timestamp"),

    // malias<mandala::est::nav::turret::cmd::pitch>("turretcmd_pitch"),
    // malias<mandala::est::nav::turret::cmd::yaw>("turretcmd_yaw"),
    // malias<mandala::est::nav::turret::ctr::roll>("turretctr_roll"),
    // malias<mandala::est::nav::turret::ctr::pitch>("turretctr_pitch"),
    // malias<mandala::est::nav::turret::ctr::yaw>("turretctr_yaw"),
    // malias<mandala::est::nav::turret::mode>("turret_mode"),
    // malias<mandala::est::nav::turret::enc::pitch>("turretenc_pitch"),
    // malias<mandala::est::nav::turret::enc::yaw>("turretenc_yaw"),

    // malias<mandala::est::nav::ats::cmd::pitch>("atscmd_pitch"),
    // malias<mandala::est::nav::ats::cmd::yaw>("atscmd_yaw"),
    // malias<mandala::est::nav::ats::ctr::pitch>("atsctr_pitch"),
    // malias<mandala::est::nav::ats::ctr::yaw>("atsctr_yaw"),
    // malias<mandala::est::nav::ats::enc::pitch>("atsenc_pitch"),
    // malias<mandala::est::nav::ats::enc::yaw>("atsenc_yaw"),
    // malias<mandala::est::nav::ats::mode>("ats_mode"),

    // malias<mandala::sns::env::gcu::rss>("gcu_RSS"),
    // malias<mandala::sns::env::gcu::ve>("gcu_Ve"),
    // malias<mandala::sns::env::gcu::mt>("gcu_MT"),

    malias<mandala::est::env::usrf::f1>("VM1"),
    malias<mandala::est::env::usrf::f2>("VM2"),
    malias<mandala::est::env::usrf::f3>("VM3"),
    malias<mandala::est::env::usrf::f4>("VM4"),
    malias<mandala::est::env::usrf::f5>("VM5"),
    malias<mandala::est::env::usrf::f6>("VM6"),
    malias<mandala::est::env::usrf::f7>("VM7"),
    malias<mandala::est::env::usrf::f8>("VM8"),
    malias<mandala::est::env::usrf::f9>("VM9"),
    malias<mandala::est::env::usrf::f10>("VM10"),
    malias<mandala::est::env::usrf::f11>("VM11"),
    malias<mandala::est::env::usrf::f12>("VM12"),
    malias<mandala::est::env::usrf::f13>("VM13"),
    malias<mandala::est::env::usrf::f14>("VM14"),
    malias<mandala::est::env::usrf::f15>("VM15"),

};

} // namespace mandala
