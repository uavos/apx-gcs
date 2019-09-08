import QtQuick 2.11

import APX.Facts 1.0

Fact {
    Fact {
        title: qsTr("Controls")
        icon: "dip-switch"
        Fact { title: qsTr("Parking brake"); bind: m.ctr_brake; dataType: Fact.Bool; }
        Fact { title: qsTr("Flaps"); bind: m.ctr_flaps; dataType: Fact.Bool; }
        Fact { title: qsTr("Interceptors"); bind: m.ctr_airbrk; dataType: Fact.Bool; }
        Fact {
            title: qsTr("Lights")
            Fact { title: qsTr("Navigation"); bind: m.sw_lights; dataType: Fact.Bool; }
            Fact { title: qsTr("Taxi"); bind: m.sw_taxi; dataType: Fact.Bool; }
        }
    }
    Fact {
        title: qsTr("Engine")
        icon: "engine"
        Fact { title: qsTr("Mixture"); bind: m.ctr_mixture; dataType: Fact.Bool; }
        Fact { title: qsTr("Ignition"); bind: m.power_ignition; dataType: Fact.Bool; }
        Fact { title: qsTr("Cut throttle"); bind: m.cmode_thrcut; dataType: Fact.Bool; }
        Fact { title: qsTr("Override throttle"); bind: m.cmode_throvr; dataType: Fact.Bool; }
        Fact { enabled: false }
        Fact { title: qsTr("Start engine"); bind: m.ctrb_starter; dataType: Fact.Bool; active: m.sw_starter.value; }
    }
    Fact {
        title: qsTr("Power")
        icon: "power-standby"
        Fact { title: qsTr("Payload"); bind: m.power_payload; dataType: Fact.Bool; }
        Fact { title: qsTr("XPDR"); bind: m.power_xpdr; dataType: Fact.Bool; }
        Fact { title: qsTr("AGL"); bind: m.power_agl; dataType: Fact.Bool; }
        Fact { enabled: false }
        Fact { title: qsTr("Servo"); bind: m.power_servo; dataType: Fact.Bool; }
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
            Fact { title: m.cmode_ahrs.descr; bind: m.cmode_ahrs; dataType: Fact.Bool; }
        }
    }
    Fact {
        title: qsTr("Emergency")
        icon: "alert-box"
        Fact { title: qsTr("ERS"); bind: m.ctrb_ers; dataType: Fact.Bool; }
        Fact { title: qsTr("Release"); bind: m.ctrb_rel; dataType: Fact.Bool; }
    }
}
