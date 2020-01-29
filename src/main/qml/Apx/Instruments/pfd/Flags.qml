import QtQuick 2.2
import "../common"

Column {

    readonly property var f_flaps: mandala.ctr.wing.flaps
    readonly property var f_brake: mandala.ctr.str.brake
    readonly property var f_ers: mandala.ctr.ers.launch
    readonly property var f_rel: mandala.ctr.ers.rel

    readonly property var f_ign: mandala.ctr.pwr.ignition
    readonly property var f_pld: mandala.ctr.pwr.payload

    readonly property var f_lights: mandala.ctr.light.taxi



    property double txtHeight
    spacing: 4
    //anchors.fill: parent
    Flag {
        id: flaps
        show: f_flaps.value > 0
        height: txtHeight
        flagColor: "cyan"
        text: qsTr("FLAPS")
        toolTip: f_flaps.descr
        //control: ctr_flaps
        Text {
            visible: ui.test || flaps.show
            color: "white"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (f_flaps.value*100).toFixed()
            font.pixelSize: height
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
    }
    Flag {
        id: brakes
        show: f_brake.value > 0
        height: txtHeight
        text: qsTr("BRAKE")
        toolTip: f_brake.descr
        //control: ctr_brake
        Text {
            visible: ui.test || (brakes.show && (f_brake.value>0) && (f_brake.value<1))
            color: "white"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (f_brake.value*100).toFixed()
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
            show: f_ers.value > 0
            visible: opacity
            height: txtHeight
            flagColor: "red"
            text: qsTr("ERS")
            toolTip: f_ers.descr
            //control: ctrb_ers
        }
        Flag {
            show: f_rel.value > 0
            //anchors.left: ers_flag.show?ers_flag.right:ers_flag.left
            //anchors.top: ers_flag.top
            height: txtHeight
            flagColor: "yellow"
            text: qsTr("REL")
            toolTip: f_rel.descr
            //control: ctrb_rel
        }
    }
    Flag {
        show: f_ign.value <= 0 && apx.datalink.valid
        height: txtHeight
        flagColor: "red"
        text: qsTr("IGN")
        toolTip: f_ign.descr
    }
    Flag {
        show: f_pld.value > 0
        height: txtHeight
        text: qsTr("PYLD")
        toolTip: f_pld.descr
    }
    Flag {
        show: f_lights.value > 0
        height: txtHeight
        text: qsTr("TAXI")
        toolTip: f_lights.descr
    }
}

