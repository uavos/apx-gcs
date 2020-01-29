import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import QtQml.Models 2.12

import Apx.Common 1.0

RowLayout {

    readonly property var f_mode: mandala.cmd.op.mode
    readonly property var f_stage: mandala.cmd.op.stage
    readonly property var f_action: mandala.cmd.op.action
    readonly property var f_adj: mandala.cmd.op.adj

    readonly property var f_flaps: mandala.ctr.wing.flaps
    readonly property var f_brake: mandala.ctr.str.brake
    readonly property var f_thr: mandala.cmd.rc.thr


    //spacing: buttonHeight/4
    ColumnLayout {
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignTop|Qt.AlignLeft
        spacing: 3
        CtrNum { title: "THR"; fact: f_thr; min: 0; max: 100; mult: 100; stepSize: 1; }
        CtrNum { title: "FLP"; fact: f_flaps; min: 0; max: 100; mult: 100; stepSize: 10; }
        CtrNum { title: "ADJ"; fact: f_adj; }
    }
    CtrFlow {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignTop
        key: f_mode.text
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
        CtrButton { title: "FLAPS"; fact: f_flaps; }
    }
    Component {
        id: btnBRK_TAXI
        CtrButton {
            title: "BRAKE"
            fact: f_brake
            color: (v>0 && v<1)?Qt.darker(Material.color(Material.Orange),1.5):v==0?Material.color(Material.Red):undefined
            width: height*3
        }
    }
    Component {
        id: btnBRK
        CtrButton {
            title: "BRAKE"
            fact: f_brake
            width: height*3
        }
    }
    Component {
        id: btnATAXI
        CtrButton {
            fact: f_action
            title: f_stage.value<=1?"AUTO":"STOP"
            value: op_action_next
            width: height*4
            highlighted: v>1
            resetValue: 100
        }
    }
    Component {
        id: btnNEXT
        CtrButton {
            fact: f_action
            title: "NEXT"
            value: op_action_next
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnINC
        CtrButton {
            fact: f_action
            title: "NEXT"
            value: op_action_inc
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnDEC
        CtrButton {
            fact: f_action
            title: "PREV"
            value: op_action_dec
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnCANCEL
        CtrButton {
            fact: f_action
            title: "CANCEL"
            value: op_action_cancel
            width: height*4
            highlighted: false
        }
    }
    Component {
        id: btnRESET
        CtrButton {
            fact: f_action
            title: "RESET"
            value: op_action_cancel
            width: height*4
            highlighted: false
        }
    }
}
