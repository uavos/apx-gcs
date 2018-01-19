import QtQuick 2.6
import QtQml 2.2
import QtQml.Models 2.2
import GCS.FactSystem 1.0
import "../../menu"

FactMenu {
    fact: FactMenuElement {
        title: qsTr("Map")
        FactMenuElement { title: qsTr("Add waypoint"); dataType: Fact.ActionData; onTriggered: app.vehicles.current.mission.waypoints.add(map.mouseClickCoordinate); }

        FactMenuElement {
            //title: qsTr("Flight Mode");
            fact: app.vehicles.current.mission
        }
        FactMenuElement {
            title: qsTr("Controls")
            FactMenuElement { title: qsTr("Parking brake"); fact: m.ctr_brake; dataType: Fact.BoolData; }
            FactMenuElement { title: qsTr("Flaps"); fact: m.ctr_flaps; dataType: Fact.BoolData; }
            FactMenuElement { title: qsTr("Interceptors"); fact: m.ctr_airbrk; dataType: Fact.BoolData; }
            FactMenuElement {
                title: qsTr("Emergency")
                FactMenuElement { title: qsTr("ERS"); fact: m.ctrb_ers; dataType: Fact.BoolData; }
                FactMenuElement { title: qsTr("Release"); fact: m.ctrb_rel; dataType: Fact.BoolData; }
            }
            FactMenuElement {
                title: qsTr("Lights")
                FactMenuElement { title: qsTr("Navigation"); fact: m.sw_lights; dataType: Fact.BoolData; }
                FactMenuElement { title: qsTr("Taxi"); fact: m.sw_taxi; dataType: Fact.BoolData; }
            }
        }
        FactMenuElement {
            title: qsTr("Engine")
            FactMenuElement { title: qsTr("Mixture"); fact: m.ctr_mixture; dataType: Fact.BoolData; }
            FactMenuElement { title: qsTr("Ignition"); fact: m.power_ignition; dataType: Fact.BoolData; }
            FactMenuElement { title: qsTr("Cut throttle"); fact: m.cmode_thrcut; dataType: Fact.BoolData; }
            FactMenuElement { title: qsTr("Override throttle"); fact: m.cmode_throvr; dataType: Fact.BoolData; }
            FactMenuElement { }
            FactMenuElement { title: qsTr("Start engine"); fact: m.ctrb_starter; dataType: Fact.BoolData; busy: m.sw_starter.value; }
        }
        FactMenuElement {
            title: qsTr("Power")
            FactMenuElement { title: qsTr("Payload"); fact: m.power_payload; dataType: Fact.BoolData; }
            FactMenuElement { title: qsTr("XPDR"); fact: m.power_xpdr; dataType: Fact.BoolData; }
            FactMenuElement { }
            FactMenuElement { title: qsTr("Servo"); fact: m.power_servo; dataType: Fact.BoolData; }
        }
        FactMenuElement {
            title: qsTr("AHRS")
            FactMenuElement { title: qsTr("Reset gps home altitude"); descr: "hmsl()"; dataType: Fact.ActionData; onTriggered: app.jsexec(descr); }
            FactMenuElement { title: qsTr("Reset static pressure"); descr: "zps()"; dataType: Fact.ActionData; onTriggered: app.jsexec(descr); }
            FactMenuElement { }
            FactMenuElement { title: m.cmode_ahrs.descr; fact: m.cmode_ahrs; dataType: Fact.BoolData; }
        }
    }
}
