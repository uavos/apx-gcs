import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import QtQml.Models 2.12

import Apx.Common 1.0

RowLayout {
    //spacing: buttonHeight/4
    ColumnLayout {
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignTop|Qt.AlignLeft
        spacing: 3
        CtrNum { title: "THR"; fact: m.rc_throttle; min: 0; max: 100; mult: 100; stepSize: 1; }
        CtrNum { title: "FLP"; fact: m.ctr_flaps; min: 0; max: 100; mult: 100; stepSize: 10; }
        CtrNum { title: "ADJ"; fact: m.rwAdj; }
    }
    CtrFlow {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignTop
        key: m.mode.text
        controls: {
            "TAXI": [btnCANCEL,btnATAXI,btnBRK_TAXI],
            "TAKEOFF": [btnCANCEL,btnNEXT,btnBRK],
            "LANDING": [btnCANCEL,btnNEXT,btnBRK],
            "WPT": [btnINC,btnDEC],
            "STBY": [btnRESET],
            "UAV": [btnNEXT,btnCANCEL],
        }
    }


    Component {
        id: btnFLP
        CtrButton { title: "FLAPS"; fact: m.ctr_flaps; }
    }
    Component {
        id: btnBRK_TAXI
        CtrButton {
            title: "BRAKE"
            fact: m.ctr_brake
            color: (v>0 && v<1)?Qt.darker(Material.color(Material.Orange),1.5):v==0?Material.color(Material.Red):undefined
            width: height*3
        }
    }
    Component {
        id: btnBRK
        CtrButton {
            title: "BRAKE"
            fact: m.ctr_brake
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
        id: btnINC
        CtrButton {
            fact: m.midx
            title: "NEXT"
            value: midx_inc
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnDEC
        CtrButton {
            fact: m.midx
            title: "PREV"
            value: midx_dec
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnCANCEL
        CtrButton {
            fact: m.stage
            title: "CANCEL"
            value: 100
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnRESET
        CtrButton {
            fact: m.stage
            title: "RESET"
            value: 100
            width: height*4
            highlighted: false
        }
    }
}
