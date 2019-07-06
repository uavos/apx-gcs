import QtQuick 2.6
import QtQml 2.2
import QtQml.Models 2.2


FactMenu {
    function openSelectUAV() { openPage({"fact": menuVehicles.fact}) }//openMenuField(menuVehicles) }
    function openServers() { openPage({"fact": menuServers.fact}) }//openMenuField(menuServers) }

    fact: FactObject {
        title: qsTr("Commands")
        FactObject {
            title: qsTr("Flight Mode");
            fact: m.mode
        }
        FactObject {
            title: qsTr("Controls")
            FactObject { title: qsTr("Parking brake"); fact: m.ctr_brake; dataType: Fact.BoolData; }
            FactObject { title: qsTr("Flaps"); fact: m.ctr_flaps; dataType: Fact.BoolData; }
            FactObject { title: qsTr("Interceptors"); fact: m.ctr_airbrk; dataType: Fact.BoolData; }
            FactObject {
                title: qsTr("Emergency")
                FactObject { title: qsTr("ERS"); fact: m.ctrb_ers; dataType: Fact.BoolData; }
                FactObject { title: qsTr("Release"); fact: m.ctrb_rel; dataType: Fact.BoolData; }
            }
            FactObject {
                title: qsTr("Lights")
                FactObject { title: qsTr("Navigation"); fact: m.sw_lights; dataType: Fact.BoolData; }
                FactObject { title: qsTr("Taxi"); fact: m.sw_taxi; dataType: Fact.BoolData; }
            }
        }
        FactObject {
            title: qsTr("Engine")
            FactObject { title: qsTr("Mixture"); fact: m.ctr_mixture; dataType: Fact.BoolData; }
            FactObject { title: qsTr("Ignition"); fact: m.power_ignition; dataType: Fact.BoolData; }
            FactObject { title: qsTr("Cut throttle"); fact: m.cmode_thrcut; dataType: Fact.BoolData; }
            FactObject { title: qsTr("Override throttle"); fact: m.cmode_throvr; dataType: Fact.BoolData; }
            FactObject { }
            FactObject { title: qsTr("Start engine"); fact: m.ctrb_starter; dataType: Fact.BoolData; busy: m.sw_starter.value; }
        }
        FactObject {
            title: qsTr("Power")
            FactObject { title: qsTr("Payload"); fact: m.power_payload; dataType: Fact.BoolData; }
            FactObject { title: qsTr("XPDR"); fact: m.power_xpdr; dataType: Fact.BoolData; }
            FactObject { }
            FactObject { title: qsTr("Servo"); fact: m.power_servo; dataType: Fact.BoolData; }
        }
        FactObject {
            title: qsTr("AHRS")
            FactObject { title: qsTr("Reset gps home altitude"); descr: "hmsl()"; dataType: Fact.ActionData; onTriggered: apx.jsexec(descr); }
            FactObject { title: qsTr("Reset static pressure"); descr: "zps()"; dataType: Fact.ActionData; onTriggered: apx.jsexec(descr); }
            FactObject { }
            FactObject { title: m.cmode_ahrs.descr; fact: m.cmode_ahrs; dataType: Fact.BoolData; }
        }
        FactObject {
            title: qsTr("Communication")
            FactObject {
                id: menuVehicles
                fact: apx.vehicles.select
                showDescr: true;
            }
            FactObject {
                id: menuServers
                fact: apx.datalink.hosts
                showDescr: true;
            }
        }
        FactObject {
            title: qsTr("Telemetry")
            FactObject { title: qsTr("Record data"); dataType: Fact.BoolData; m_value: apx.vehicles.current.recorder.recording; onValueUpdated:  apx.vehicles.current.recorder.recording=v; }
            FactObject { title: qsTr("Discard current file"); dataType: Fact.ActionData; onTriggered: apx.vehicles.current.recorder.discard(); }
        }
        FactObject { }
        FactObject { title: qsTr("RESET")+" "+m.mode.text; dataType: Fact.ActionData; onTriggered: m.stage.setValue(100); busy: m.stage.value===100; }
        FactObject { title: qsTr("NEXT STAGE")+" ("+m.stage.value+")"; dataType: Fact.ActionData; onTriggered: m.stage.setValue(m.stage.value+1); }
    }
}
