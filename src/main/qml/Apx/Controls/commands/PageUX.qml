import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

Page {
    ColumnLayout {
        CtrFlow {
            Layout.minimumWidth: pagesView.width
            key: m.mode.text
            controls: {
                "UAV": [ctrFLP],
                "WPT": [ctrFLP,ctrADJ],
                "STBY": [ctrFLP,ctrADJ],
                "TAXI": [ctrTHR,ctrADJ],
                "TAKEOFF": [ctrADJ],
                "LANDING": [ctrFLP,ctrADJ],
            }
        }

        CtrFlow {
            Layout.minimumWidth: pagesView.width
            key: m.mode.text
            controls: {
                "TAXI": [btnATAXI,btnBRK],
                "TAKEOFF": [btnNEXT,btnFLP,btnBRK],
                "LANDING": [btnCANCEL,btnNEXT,btnBRK],
            }
        }
    }

    Component {
        id: ctrADJ
        CtrNum { title: "ADJ"; fact: m.rwAdj; }
    }
    Component {
        id: ctrFLP
        CtrNum { title: "FLP"; fact: m.ctr_flaps; min: 0; max: 100; mult: 100; stepSize: 10; }
    }
    Component {
        id: ctrTHR
        CtrNum { title: "THR"; fact: m.rc_throttle; min: 0; max: 100; mult: 100; stepSize: 1; }
    }


    Component {
        id: btnFLP
        CtrButton { title: "FLAPS"; fact: m.ctr_flaps; }
    }
    Component {
        id: btnBRK
        CtrButton {
            title: "BRAKE"
            fact: m.ctr_brake
            color: (v>0 && v<1)?Qt.darker(Material.color(Material.Orange),1.5):v==0?Material.color(Material.Red):undefined
            width: height*3
        }
    }
    Component {
        id: btnATAXI
        CtrButton {
            fact: m.stage
            title: v<=1?"AUTO":"STOP"
            value: v+1
            width: height*4
            //size: root.size
            highlighted: v>1
            resetValue: 100
        }
    }
    Component {
        id: btnNEXT
        CtrButton {
            fact: m.stage
            title: "NEXT"
            value: v+1
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnCANCEL
        CtrButton {
            fact: m.stage
            title: "CANCEL"
            enabled: value<100
            value: 100
            width: height*4
            highlighted: false
        }
    }
}
