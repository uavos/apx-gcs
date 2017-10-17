import QtQuick 2.6
import QtQml 2.2
import "."

GCSMenu {
    id: menuCommands
    title: qsTr("Commands")

    function openSelectUAV() { openMenuField(menuVehicles) }
    function openServers() { openMenuField(menuServers) }


    fields: GCSMenuModel {
        GCSMenuField {
            title: qsTr("Flight Mode");
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("EMG"); onClicked: mode.setValue(mode_EMG); }
                GCSMenuField { title: qsTr("RPV"); onClicked: mode.setValue(mode_RPV); }
                GCSMenuField { title: qsTr("UAV"); onClicked: mode.setValue(mode_UAV); }
                GCSMenuField { title: qsTr("WPT"); onClicked: mode.setValue(mode_WPT); }
                GCSMenuField { title: qsTr("HOME"); onClicked: mode.setValue(mode_HOME); }
                GCSMenuField { title: qsTr("STBY"); onClicked: mode.setValue(mode_STBY); }
                GCSMenuField { title: qsTr("TAXI"); onClicked: mode.setValue(mode_TAXI); }
                GCSMenuField { title: qsTr("TAKEOFF"); onClicked: mode.setValue(mode_TAKEOFF); }
                GCSMenuField { title: qsTr("LANDING"); onClicked: mode.setValue(mode_LANDING); }
            }
        }
        GCSMenuField {
            title: qsTr("Controls")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Parking brake"); onClicked: ctr_brake.setValue(!ctr_brake.value); checked: ctr_brake.value; checkable: true; }
                GCSMenuField { title: qsTr("Flaps"); onClicked: ctr_flaps.setValue(!ctr_flaps.value); checked: ctr_flaps.value; checkable: true; }
                GCSMenuField { title: qsTr("Interceptors"); onClicked: ctr_airbrk.setValue(!ctr_airbrk.value); checked: ctr_airbrk.value; checkable: true; }
                GCSMenuField {
                    title: qsTr("Emergency")
                    fields: GCSMenuModel {
                        GCSMenuField { title: qsTr("ERS"); onClicked: ctrb_ers.setValue(!ctrb_ers.value); checked: ctrb_ers.value; checkable: true; }
                        GCSMenuField { title: qsTr("Release"); onClicked: ctrb_rel.setValue(!ctrb_rel.value); checked: ctrb_rel.value; checkable: true; }
                    }
                }
                GCSMenuField {
                    title: qsTr("Lights")
                    fields: GCSMenuModel {
                        GCSMenuField { title: qsTr("Navigation"); onClicked: sw_lights.setValue(!sw_lights.value); checked: sw_lights.value; checkable: true; }
                        GCSMenuField { title: qsTr("Taxi"); onClicked: sw_taxi.setValue(!sw_taxi.value); checked: sw_taxi.value; checkable: true; }
                    }
                }
            }
        }
        GCSMenuField {
            title: qsTr("Engine")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Mixture"); onClicked: ctr_mixture.setValue(!ctr_mixture.value); checked: ctr_mixture.value; checkable: true; }
                GCSMenuField { title: qsTr("Ignition"); onClicked: power_ignition.setValue(!power_ignition.value); checked: power_ignition.value; checkable: true; }
                GCSMenuField { title: qsTr("Cut throttle"); onClicked: cmode_thrcut.setValue(!cmode_thrcut.value); checked: cmode_thrcut.value; checkable: true;}
                GCSMenuField { title: qsTr("Override throttle"); onClicked: cmode_throvr.setValue(!cmode_throvr.value); checked: cmode_throvr.value; checkable: true;}
                GCSMenuField { separator: true;}
                GCSMenuField { title: qsTr("Start engine"); onClicked: ctrb_starter.setValue(!ctrb_starter.value); busy: sw_starter.value; }
            }
        }
        GCSMenuField {
            title: qsTr("Power")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Servo"); onClicked: power_servo.setValue(!power_servo.value); checked: power_servo.value; checkable: true; }
                GCSMenuField { title: qsTr("Payload"); onClicked: power_payload.setValue(!power_payload.value); checked: power_payload.value; checkable: true; }
                GCSMenuField { title: qsTr("XPDR"); onClicked: power_xpdr.setValue(!power_xpdr.value); checked: power_xpdr.value; checkable: true; }
            }
        }
        GCSMenuField { separator: true; }
        GCSMenuField {
            title: qsTr("AHRS")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Reset gps home altitude"); onClicked: mandala.current.exec_script("hmsl()"); }
                GCSMenuField { title: qsTr("Reset static pressure"); onClicked: mandala.current.exec_script("zps()"); }
                GCSMenuField { separator: true;}
                GCSMenuField { title: cmode_ahrs.descr; onClicked: cmode_ahrs.setValue(!cmode_ahrs.value); checked: cmode_ahrs.value; checkable: true;}
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
                    title: qsTr("Servers")
                    fields: GCSMenuModel {
                        id: objModelServers
                        GCSMenuField { title: qsTr("Disconnect"); onClicked: datalink.disconnect(); }
                        GCSMenuList {
                            objModel: objModelServers
                            model: datalink.serverNames
                            delegate: GCSMenuField {
                                title: model.modelData
                                onClicked: datalink.connectToServer(title)
                            }
                        }
                    }
                }
            }
        }
        GCSMenuField {
            title: qsTr("Telemetry")
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Record data"); onClicked: mandala.current.recorder.recording=!mandala.current.recorder.recording; checked: mandala.current.recorder.recording; checkable: true;}
                GCSMenuField { title: qsTr("Discard current file"); onClicked: mandala.current.recorder.discard(); }
            }
        }
        GCSMenuField { separator: true; }
        GCSMenuField { title: qsTr("RESET")+" "+mode.text; onClicked: stage.setValue(100); busy: stage.value===100; }
        GCSMenuField { title: qsTr("NEXT STAGE")+" ("+stage.value+")"; onClicked: stage.setValue(stage.value+1); }
    }
}
