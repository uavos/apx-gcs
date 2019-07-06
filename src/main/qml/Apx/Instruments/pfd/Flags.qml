import QtQuick 2.2
import "../common"

Column {
    property double txtHeight
    spacing: 4
    //anchors.fill: parent
    Flag {
        id: flaps
        show: m.ctr_flaps.value > 0
        height: txtHeight
        flagColor: "cyan"
        text: qsTr("FLAPS")
        toolTip: m.ctr_flaps.descr
        //control: ctr_flaps
        Text {
            visible: ui.test || flaps.show
            color: "white"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (m.ctr_flaps.value*100).toFixed()
            font.pixelSize: height
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
    }
    Flag {
        id: brakes
        show: m.ctr_brake.value > 0
        height: txtHeight
        text: qsTr("BRAKE")
        toolTip: m.ctr_brake.descr
        //control: ctr_brake
        Text {
            visible: ui.test || (brakes.show && (m.ctr_brake.value>0) && (m.ctr_brake.value<1))
            color: "white"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (m.ctr_brake.value*100).toFixed()
            font.pixelSize: height
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
    }
    Row {
        spacing: 1
        Flag {
            id: ers_flag
            show: m.ctrb_ers.value > 0
            visible: opacity
            height: txtHeight
            flagColor: "red"
            text: qsTr("ERS")
            toolTip: m.ctrb_ers.descr
            //control: ctrb_ers
        }
        Flag {
            show: m.ctrb_rel.value > 0
            //anchors.left: ers_flag.show?ers_flag.right:ers_flag.left
            //anchors.top: ers_flag.top
            height: txtHeight
            flagColor: "yellow"
            text: qsTr("REL")
            toolTip: m.ctrb_rel.descr
            //control: ctrb_rel
        }
    }
    Flag {
        show: m.power_ignition.value <= 0 && apx.datalink.valid
        height: txtHeight
        flagColor: "red"
        text: qsTr("IGN")
        toolTip: m.power_ignition.descr
        //control: power_ignition
    }
    Flag {
        show: m.power_payload.value > 0
        height: txtHeight
        text: qsTr("PYLD")
        toolTip: m.power_payload.descr
        //control: power_payload
    }
    Flag {
        show: m.sw_taxi.value > 0
        height: txtHeight
        text: qsTr("TAXI")
        toolTip: m.sw_taxi.descr
        //control: sw_taxi
    }
    /*Flag {
        show: m.sw_lights.value > 0
        height: txtHeight
        text: qsTr("LGHT")
        toolTip: m.cmode_throvr.descr
    }*/
}

