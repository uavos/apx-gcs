import QtQuick 2.11

import APX.Facts 1.0

Fact {
    Fact {
        title: qsTr("Controls")
        icon: "dip-switch"
        Fact { title: qsTr("Parking brake"); bind: m.ctr_brake; flags: Fact.Bool; }
        Fact { title: qsTr("Flaps"); bind: m.ctr_flaps; flags: Fact.Bool; }
        Fact { title: qsTr("Interceptors"); bind: m.ctr_airbrk; flags: Fact.Bool; }
        Fact {
            title: qsTr("Lights")
            Fact { title: qsTr("Navigation"); bind: m.sw_lights; flags: Fact.Bool; }
            Fact { title: qsTr("Taxi"); bind: m.sw_taxi; flags: Fact.Bool; }
        }
    }
    Fact {
        title: qsTr("Engine")
        icon: "engine"
        Fact { title: qsTr("Mixture"); bind: m.ctr_mixture; flags: Fact.Bool; }
        Fact { title: qsTr("Ignition"); bind: m.power_ignition; flags: Fact.Bool; }
        Fact { title: qsTr("Cut throttle"); bind: m.cmode_thrcut; flags: Fact.Bool; }
        Fact { title: qsTr("Override throttle"); bind: m.cmode_throvr; flags: Fact.Bool; }
        Fact { enabled: false }
        Fact { title: qsTr("Start engine"); bind: m.ctrb_starter; flags: Fact.Bool; active: m.sw_starter.value; }
    }
    Fact {
        title: qsTr("Power")
        icon: "power-standby"
        Fact { title: qsTr("Payload"); bind: m.power_payload; flags: Fact.Bool; }
        Fact { title: qsTr("XPDR"); bind: m.power_xpdr; flags: Fact.Bool; }
        Fact { title: qsTr("AGL"); bind: m.power_agl; flags: Fact.Bool; }
        Fact { enabled: false }
        Fact { title: qsTr("Servo"); bind: m.power_servo; flags: Fact.Bool; }
    }
    Fact {
        title: qsTr("Service")
        icon: "settings"
        Fact {
            title: qsTr("AHRS")
            Fact { title: qsTr("Reset gps home altitude"); descr: "hmsl()"; onTriggered: application.jsexec(descr); }
            Fact { title: qsTr("Reset static pressure"); descr: "zps()"; onTriggered: application.jsexec(descr); }
            Fact { enabled: false }
            Fact { title: qsTr("Reset AHRS filter"); descr: "ahrs()"; onTriggered: application.jsexec(descr); }
            Fact { enabled: false }
            Fact { title: m.cmode_ahrs.descr; bind: m.cmode_ahrs; flags: Fact.Bool; }
        }
    }
    Fact {
        title: qsTr("Emergency")
        icon: "alert-box"
        Fact { title: qsTr("ERS"); bind: m.ctrb_ers; flags: Fact.Bool; }
        Fact { title: qsTr("Release"); bind: m.ctrb_rel; flags: Fact.Bool; }
    }
}
