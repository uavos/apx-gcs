import QtQuick 2.11

import APX.Facts 1.0
import Apx.Common 1.0

FactObject {
    FactObject {
        title: qsTr("Controls")
        icon: "dip-switch" //"google-controller"
        FactObject { title: qsTr("Parking brake"); fact: m.ctr_brake; dataType: Fact.Bool; }
        FactObject { title: qsTr("Flaps"); fact: m.ctr_flaps; dataType: Fact.Bool; }
        FactObject { title: qsTr("Interceptors"); fact: m.ctr_airbrk; dataType: Fact.Bool; }
        FactObject {
            title: qsTr("Lights")
            FactObject { title: qsTr("Navigation"); fact: m.sw_lights; dataType: Fact.Bool; }
            FactObject { title: qsTr("Taxi"); fact: m.sw_taxi; dataType: Fact.Bool; }
        }
    }
    FactObject {
        title: qsTr("Engine")
        icon: "engine"
        FactObject { title: qsTr("Mixture"); fact: m.ctr_mixture; dataType: Fact.Bool; }
        FactObject { title: qsTr("Ignition"); fact: m.power_ignition; dataType: Fact.Bool; }
        FactObject { title: qsTr("Cut throttle"); fact: m.cmode_thrcut; dataType: Fact.Bool; }
        FactObject { title: qsTr("Override throttle"); fact: m.cmode_throvr; dataType: Fact.Bool; }
        FactObject { }
        FactObject { title: qsTr("Start engine"); fact: m.ctrb_starter; dataType: Fact.Bool; active: m.sw_starter.value; }
    }
    FactObject {
        title: qsTr("Power")
        icon: "power-standby"
        FactObject { title: qsTr("Payload"); fact: m.power_payload; dataType: Fact.Bool; }
        FactObject { title: qsTr("XPDR"); fact: m.power_xpdr; dataType: Fact.Bool; }
        FactObject { title: qsTr("AGL"); fact: m.power_agl; dataType: Fact.Bool; }
        FactObject { }
        FactObject { title: qsTr("Servo"); fact: m.power_servo; dataType: Fact.Bool; }
    }
    FactObject {
        title: qsTr("Service")
        icon: "settings"
        FactObject {
            title: qsTr("AHRS")
            FactObject { title: qsTr("Reset gps home altitude"); descr: "hmsl()"; onTriggered: application.jsexec(descr); }
            FactObject { title: qsTr("Reset static pressure"); descr: "zps()"; onTriggered: application.jsexec(descr); }
            FactObject { }
            FactObject { title: qsTr("Reset AHRS filter"); descr: "ahrs()"; onTriggered: application.jsexec(descr); }
            FactObject { }
            FactObject { title: m.cmode_ahrs.descr; fact: m.cmode_ahrs; dataType: Fact.Bool; }
        }
    }
    FactObject {
        title: qsTr("Emergency")
        icon: "alert-box"
        FactObject { title: qsTr("ERS"); fact: m.ctrb_ers; dataType: Fact.Bool; }
        FactObject { title: qsTr("Release"); fact: m.ctrb_rel; dataType: Fact.Bool; }
    }
}
