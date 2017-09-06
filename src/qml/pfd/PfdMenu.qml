import QtQuick 2.6
//import QtQuick.Controls 2.1
//import QtQuick.Controls.Material 2.1
//import QtQuick 2.2
//import QtQuick.Controls 1.1
import "."
import "../components"

PopupMenu {
    //id: menu
    title: qsTr("Commands")

    function openSelectUAV() { openPage(menuSelectUAV) }
    function openServers() { openPage(menuServers) }


    PopupMenuItem {
        title: qsTr("Flight Mode");
        PopupMenuItem { title: qsTr("EMG"); onClicked: mode.setValue(mode_EMG); }
        PopupMenuItem { title: qsTr("RPV"); onClicked: mode.setValue(mode_RPV); }
        PopupMenuItem { title: qsTr("UAV"); onClicked: mode.setValue(mode_UAV); }
        PopupMenuItem { title: qsTr("WPT"); onClicked: mode.setValue(mode_WPT); }
        PopupMenuItem { title: qsTr("HOME"); onClicked: mode.setValue(mode_HOME); }
        PopupMenuItem { title: qsTr("STBY"); onClicked: mode.setValue(mode_STBY); }
        PopupMenuItem { title: qsTr("TAXI"); onClicked: mode.setValue(mode_TAXI); }
        PopupMenuItem { title: qsTr("TAKEOFF"); onClicked: mode.setValue(mode_TAKEOFF); }
        PopupMenuItem { title: qsTr("LANDING"); onClicked: mode.setValue(mode_LANDING); }
    }
    PopupMenuItem {
        title: qsTr("Controls")
        PopupMenuItem { title: qsTr("Parking brake"); onClicked: ctr_brake.setValue(!ctr_brake.value); checked: ctr_brake.value; checkable: true; }
        PopupMenuItem { title: qsTr("Flaps"); onClicked: ctr_flaps.setValue(!ctr_flaps.value); checked: ctr_flaps.value; checkable: true; }
        PopupMenuItem { title: qsTr("Interceptors"); onClicked: ctr_airbrk.setValue(!ctr_airbrk.value); checked: ctr_airbrk.value; checkable: true; }
        PopupMenuItem {
            title: qsTr("Emergency")
            PopupMenuItem { title: qsTr("ERS"); onClicked: ctrb_ers.setValue(!ctrb_ers.value); checked: ctrb_ers.value; checkable: true; }
            PopupMenuItem { title: qsTr("Release"); onClicked: ctrb_rel.setValue(!ctrb_rel.value); checked: ctrb_rel.value; checkable: true; }
        }
        PopupMenuItem {
            title: qsTr("Lights")
            PopupMenuItem { title: qsTr("Navigation"); onClicked: sw_lights.setValue(!sw_lights.value); checked: sw_lights.value; checkable: true; }
            PopupMenuItem { title: qsTr("Taxi"); onClicked: sw_taxi.setValue(!sw_taxi.value); checked: sw_taxi.value; checkable: true; }
        }

    }
    PopupMenuItem {
        title: qsTr("Engine")
        PopupMenuItem { title: qsTr("Mixture"); onClicked: ctr_mixture.setValue(!ctr_mixture.value); checked: ctr_mixture.value; checkable: true; }
        PopupMenuItem { title: qsTr("Ignition"); onClicked: power_ignition.setValue(!power_ignition.value); checked: power_ignition.value; checkable: true; }
        PopupMenuItem { title: qsTr("Cut throttle"); onClicked: cmode_thrcut.setValue(!cmode_thrcut.value); checked: cmode_thrcut.value; checkable: true;}
        PopupMenuItem { title: qsTr("Override throttle"); onClicked: cmode_throvr.setValue(!cmode_throvr.value); checked: cmode_throvr.value; checkable: true;}
        PopupMenuItem { }
        PopupMenuItem { title: qsTr("Start engine"); onClicked: ctrb_starter.setValue(!ctrb_starter.value); }
    }
    PopupMenuItem {
        title: qsTr("Power")
        PopupMenuItem { title: qsTr("Servo"); onClicked: power_servo.setValue(!power_servo.value); checked: power_servo.value; checkable: true; }
        PopupMenuItem { title: qsTr("Payload"); onClicked: power_payload.setValue(!power_payload.value); checked: power_payload.value; checkable: true; }
        PopupMenuItem { title: qsTr("XPDR"); onClicked: power_xpdr.setValue(!power_xpdr.value); checked: power_xpdr.value; checkable: true; }
    }
    PopupMenuItem { }
    PopupMenuItem {
        title: qsTr("AHRS")
        PopupMenuItem { title: qsTr("Reset gps home altitude"); onClicked: mandala.current.exec_script("hmsl()"); }
        PopupMenuItem { title: qsTr("Reset static pressure"); onClicked: mandala.current.exec_script("zps()"); }
        PopupMenuItem { }
        PopupMenuItem { title: cmode_ahrs.descr; onClicked: cmode_ahrs.setValue(!cmode_ahrs.value); checked: cmode_ahrs.value; checkable: true;}
    }
    PopupMenuItem { }
    PopupMenuItem {
        title: qsTr("Communication")
        PopupMenuItem {
            title: qsTr("Select Vehicle")
            id: menuSelectUAV
            property var list: Instantiator {
                model: mandala.uavNames
                PopupMenuItem {
                    title: model.modelData
                    onClicked: mandala.setCurrent(model.modelData)
                }
                onObjectAdded: menuSelectUAV.appendItem(object)
                //onObjectRemoved: menuSelectUAV.removeItem(object)
            }
        }
        PopupMenuItem {
            title: qsTr("Servers")
            id: menuServers
            PopupMenuItem { title: qsTr("Disconnect"); onClicked: datalink.disconnect(); }
            PopupMenuItem { }
            property var list: Instantiator {
                model: datalink.serverNames
                PopupMenuItem {
                    title: model.modelData
                    onClicked: datalink.connectToServer(title)
                }
                onObjectAdded: { menuServers.appendItem(object); }
            }
        }
        /*PopupMenuItem {
            title: qsTr("Test")
            onClicked: {
                var portID=128;
                var buf=[1,2,3,4,5];
                var sbuf="";
                for (var i=0; i < buf.length; i++) {
                    sbuf+=(i===0?"'":",")+buf[i].toFixed();
                }
                sbuf+="'";
                mandala.current.exec_script("serial("+portID.toFixed()+","+sbuf+")");
            }
        }*/
    }
    PopupMenuItem { }
    PopupMenuItem { title: qsTr("CANCEL")+" "+mode.text; onClicked: stage.setValue(100); }
    PopupMenuItem { }
    PopupMenuItem { title: qsTr("NEXT STAGE")+" ("+stage.value+")"; onClicked: stage.setValue(stage.value+1); }
}
