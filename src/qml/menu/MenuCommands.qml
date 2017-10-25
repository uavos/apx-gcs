import QtQuick 2.6
import QtQml 2.2
import "."

GCSMenu {
    id: menuCommands
    title: qsTr("Commands")

    function openSelectUAV() { openMenuField(menuVehicles) }
    function openServers() { openPage({"fact": menuServers.fact}) }//openMenuField(menuServers) }


    fields: GCSMenuModel {
        GCSMenuField {
            title: qsTr("Flight Mode");
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("EMG"); onClicked: m.mode.setValue(mode_EMG); }
                GCSMenuField { title: qsTr("RPV"); onClicked: m.mode.setValue(mode_RPV); }
                GCSMenuField { title: qsTr("UAV"); onClicked: m.mode.setValue(mode_UAV); }
                GCSMenuField { title: qsTr("WPT"); onClicked: m.mode.setValue(mode_WPT); }
                GCSMenuField { title: qsTr("HOME"); onClicked: m.mode.setValue(mode_HOME); }
                GCSMenuField { title: qsTr("STBY"); onClicked: m.mode.setValue(mode_STBY); }
                GCSMenuField { title: qsTr("TAXI"); onClicked: m.mode.setValue(mode_TAXI); }
                GCSMenuField { title: qsTr("TAKEOFF"); onClicked: m.mode.setValue(mode_TAKEOFF); }
                GCSMenuField { title: qsTr("LANDING"); onClicked: m.mode.setValue(mode_LANDING); }
            }
        }
        GCSMenuField {
            title: qsTr("Controls")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Parking brake"); onToggled: m.ctr_brake.setValue(!m.ctr_brake.value); checked: m.ctr_brake.value; checkable: true; }
                GCSMenuField { title: qsTr("Flaps"); onToggled: m.ctr_flaps.setValue(!m.ctr_flaps.value); checked: m.ctr_flaps.value; checkable: true; }
                GCSMenuField { title: qsTr("Interceptors"); onToggled: m.ctr_airbrk.setValue(!m.ctr_airbrk.value); checked: m.ctr_airbrk.value; checkable: true; }
                GCSMenuField {
                    title: qsTr("Emergency")
                    fields: GCSMenuModel {
                        GCSMenuField { title: qsTr("ERS"); onToggled: m.ctrb_ers.setValue(!m.ctrb_ers.value); checked: m.ctrb_ers.value; checkable: true; }
                        GCSMenuField { title: qsTr("Release"); onToggled: m.ctrb_rel.setValue(!m.ctrb_rel.value); checked: m.ctrb_rel.value; checkable: true; }
                    }
                }
                GCSMenuField {
                    title: qsTr("Lights")
                    fields: GCSMenuModel {
                        GCSMenuField { title: qsTr("Navigation"); onToggled: m.sw_lights.setValue(!m.sw_lights.value); checked: m.sw_lights.value; checkable: true; }
                        GCSMenuField { title: qsTr("Taxi"); onToggled: m.sw_taxi.setValue(!m.sw_taxi.value); checked: m.sw_taxi.value; checkable: true; }
                    }
                }
            }
        }
        GCSMenuField {
            title: qsTr("Engine")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Mixture"); onToggled: m.ctr_mixture.setValue(!m.ctr_mixture.value); checked: m.ctr_mixture.value; checkable: true; }
                GCSMenuField { title: qsTr("Ignition"); onToggled: m.power_ignition.setValue(!m.power_ignition.value); checked: m.power_ignition.value; checkable: true; }
                GCSMenuField { title: qsTr("Cut throttle"); onToggled: m.cmode_thrcut.setValue(!m.cmode_thrcut.value); checked: m.cmode_thrcut.value; checkable: true;}
                GCSMenuField { title: qsTr("Override throttle"); onToggled: m.cmode_throvr.setValue(!m.cmode_throvr.value); checked: m.cmode_throvr.value; checkable: true;}
                GCSMenuField { separator: true;}
                GCSMenuField { title: qsTr("Start engine"); onClicked: m.ctrb_starter.setValue(!m.ctrb_starter.value); busy: m.sw_starter.value; }
            }
        }
        GCSMenuField {
            title: qsTr("Power")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Servo"); onToggled: m.power_servo.setValue(!m.power_servo.value); checked: m.power_servo.value; checkable: true; }
                GCSMenuField { title: qsTr("Payload"); onToggled: m.power_payload.setValue(!m.power_payload.value); checked: m.power_payload.value; checkable: true; }
                GCSMenuField { title: qsTr("XPDR"); onToggled: m.power_xpdr.setValue(!m.power_xpdr.value); checked: m.power_xpdr.value; checkable: true; }
            }
        }
        GCSMenuField { separator: true; }
        GCSMenuField {
            title: qsTr("AHRS")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Reset gps home altitude"); onClicked: mandala.current.exec_script("hmsl()"); }
                GCSMenuField { title: qsTr("Reset static pressure"); onClicked: mandala.current.exec_script("zps()"); }
                GCSMenuField { separator: true;}
                GCSMenuField { title: m.cmode_ahrs.descr; onToggled: m.cmode_ahrs.setValue(!m.cmode_ahrs.value); checked: m.cmode_ahrs.value; checkable: true;}
            }
        }
        GCSMenuField { separator: true; }
        GCSMenuField {
            title: qsTr("Communication")
            fields: GCSMenuModel {
                GCSMenuField {
                    id: menuVehicles
                    title: qsTr("Select Vehicle")
                    fields: GCSMenuModel {
                        id: objModelVehicles
                        GCSMenuList {
                            objModel: objModelVehicles
                            //model: mandala.uavNames
                            delegate: GCSMenuField {
                                title: model.modelData
                                onClicked: mandala.setCurrent(model.modelData)
                            }
                        }
                    }
                }
                GCSMenuField {
                    id: menuServers
                    fact: app.datalink.hosts
                }
            }
        }
        GCSMenuField {
            title: qsTr("Telemetry")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Record data"); onToggled: mandala.current.recorder.recording=!mandala.current.recorder.recording; checked: mandala.current.recorder.recording; checkable: true;}
                GCSMenuField { title: qsTr("Discard current file"); onClicked: mandala.current.recorder.discard(); }
            }
        }
        GCSMenuField { separator: true; }
        GCSMenuField { title: qsTr("RESET")+" "+m.mode.text; onClicked: m.stage.setValue(100); busy: m.stage.value===100; }
        GCSMenuField { title: qsTr("NEXT STAGE")+" ("+m.stage.value+")"; onClicked: m.stage.setValue(m.stage.value+1); }
    }
}
