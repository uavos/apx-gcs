import QtQuick 2.2
import "../common"

Column {

    readonly property var f_flaps: mandala.ctr.wing.flaps
    readonly property var f_brake: mandala.ctr.str.brake
    readonly property var f_ers: mandala.ctr.ers.launch
    readonly property var f_rel: mandala.ctr.ers.rel

    readonly property var f_ign: mandala.ctr.pwr.ignition
    readonly property var f_pld: mandala.ctr.pwr.payload

    //readonly property var f_lights: mandala.ctr.light.taxi



    property double txtHeight
    spacing: 4
    //anchors.fill: parent
    StatusFlag {
        id: flaps
        height: txtHeight
        visible: true
        fact: f_flaps
        text: qsTr("FLAPS")
        readonly property real v: fact.value
        show: v > 0
        status_warning: 0.6
        CleanText {
            height: txtHeight
            fact: flaps.fact
            show: flaps.show
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (flaps.v*100).toFixed()
        }
    }
    StatusFlag {
        id: brakes
        height: txtHeight
        visible: true
        fact: f_brake
        text: qsTr("BRAKE")
        readonly property real v: fact.value
        show: v > 0
        type: CleanText.Yellow
        CleanText {
            height: txtHeight
            fact: brakes.fact
            show: brakes.show && brakes.v < 1
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (brakes.v*100).toFixed()
        }
    }
    Row {
        spacing: 1
        height: txtHeight
        StatusFlag {
            id: ers_flag
            height: txtHeight
            fact: f_ers
            show: fact.value > 0
            type: CleanText.Red
            text: qsTr("ERS")
        }
        StatusFlag {
            height: txtHeight
            fact: f_rel
            show: fact.value > 0
            type: CleanText.Yellow
            text: qsTr("REL")
        }
    }
    StatusFlag {
        height: txtHeight
        visible: true
        fact: f_ign
        show: fact.value <= 0 && apx.datalink.valid
        type: CleanText.Red
        text: qsTr("IGN")
    }
    StatusFlag {
        height: txtHeight
        visible: true
        fact: f_pld
        show: fact.value > 0
        type: CleanText.Green
        text: qsTr("PYLD")
    }
    /*StatusFlag {
        height: txtHeight
        visible: true
        fact: f_lights
        show: fact.value > 0
        text: qsTr("TAXI")
    }*/
}

