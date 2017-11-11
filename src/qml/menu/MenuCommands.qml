import QtQuick 2.6
import QtQml 2.2
import QtQml.Models 2.2
import GCS.FactSystem 1.0
import "."

FactMenu {
    function openSelectUAV() { openPage({"fact": menuVehicles.fact}) }//openMenuField(menuVehicles) }
    function openServers() { openPage({"fact": menuServers.fact}) }//openMenuField(menuServers) }

    fact: FactMenuElement {
        title: qsTr("Commands")
        FactMenuElement {
            title: qsTr("Flight Mode");
            fact: m.mode
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
        FactMenuElement {
            title: qsTr("Communication")
            FactMenuElement {
                id: menuVehicles
                fact: app.vehicles.select
                showDescr: true;
            }
            FactMenuElement {
                id: menuServers
                fact: app.datalink.hosts
                showDescr: true;
            }
        }
        FactMenuElement {
            title: qsTr("Telemetry")
            FactMenuElement { title: qsTr("Record data"); dataType: Fact.BoolData; m_value: app.vehicles.current.recorder.recording; onValueUpdated:  app.vehicles.current.recorder.recording=v; }
            FactMenuElement { title: qsTr("Discard current file"); dataType: Fact.ActionData; onTriggered: app.vehicles.current.recorder.discard(); }
        }
        FactMenuElement { }
        FactMenuElement { title: qsTr("RESET")+" "+m.mode.text; dataType: Fact.ActionData; onTriggered: m.stage.setValue(100); busy: m.stage.value===100; }
        FactMenuElement { title: qsTr("NEXT STAGE")+" ("+m.stage.value+")"; dataType: Fact.ActionData; onTriggered: m.stage.setValue(m.stage.value+1); }
    }
}
