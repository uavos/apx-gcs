import QtQuick 2.2
import "../common"

ControlArea {


    readonly property var f_airspeed: mandala.est.air.airspeed
    readonly property var f_cmd_airspeed: mandala.cmd.air.airspeed
    readonly property var f_speed: mandala.est.pos.speed


    mvar: f_cmd_airspeed   //ControlArea
    span: 10
    min:0
    max: 100
    step: 1
    stepDrag: step*0.5
    stepLimit: 1
    fixedPoint: true
    doWheel: false

    //instrument item
    property double anumation_duration: 200

    Rectangle {
        id: speed_window
        color: "transparent"
        border.width: 0
        clip: true
        anchors.fill: parent
        anchors.verticalCenter: parent.verticalCenter

        property double strip_width: 0.3

        property real speed_value: f_airspeed.value
        Behavior on speed_value { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }

        property double strip_scale: width*strip_width/svgRenderer.elementBounds(pfdImageUrl, "speed-scale").width
        property double num2scaleHeight:
            svgRenderer.elementBounds(pfdImageUrl, "speed-scale").height * strip_scale /10

        Item {
            id: speed_scale
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width: parent.width*speed_window.strip_width

            property int item_cnt: Math.floor(speed_window.height/speed_window.num2scaleHeight*10/2)*2+2
            Repeater {
                model: 8    //speed_scale.item_cnt
                PfdImage {
                    id: speed_scale_image
                    elementName: "speed-scale"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 1
                    anchors.right: parent.right
                    anchors.verticalCenterOffset: (index-2)*height + height/10 * (speed_window.speed_value-Math.floor(speed_window.speed_value/1)*1)
                    width: parent.width
                    height: elementBounds.height*speed_window.strip_scale
                }
            }
        }

        Item {
            id: scale_numbers

            anchors.right: parent.right
            anchors.rightMargin: speed_window.width*speed_window.strip_width
            anchors.verticalCenter: parent.verticalCenter

            property int item_cnt: Math.floor(speed_window.height/speed_window.num2scaleHeight/2)*2+2
            property int topNumber: Math.floor(speed_window.speed_value)+item_cnt/2
            Repeater {
                model: scale_numbers.item_cnt
                Item {
                    width: scale_numbers.width
                    transform: [
                        Scale { yScale: 1.01 }, //force render as img
                        Translate { y: (index-scale_numbers.item_cnt/2+(speed_window.speed_value-Math.floor(speed_window.speed_value)))*speed_window.num2scaleHeight }
                    ]

                    Text {
                        property int num: scale_numbers.topNumber - index
                        smooth: ui.antialiasing
                        text: num
                        visible: num>=0
                        //render as image
                        style: Text.Raised
                        styleColor: "transparent"

                        color: "#A0FFFFFF"
                        font.family: font_condenced
                        font.bold: true
                        font.pixelSize: speed_window.num2scaleHeight / 1.5
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        PfdImage {
            id: speed_waypoint
            elementName: "speed-waypoint"
            width: elementBounds.width*height/elementBounds.height  //speed_window.strip_scale
            height: speed_box.height    //elementBounds.height*speed_window.strip_scale

            anchors.right: speed_scale.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: apx.limit(speed_window.num2scaleHeight * (f_airspeed.value - f_cmd_airspeed.value),-parent.height/2,parent.height/2)
            Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
            ToolTipArea {text: f_cmd_airspeed.descr}
        }

        PfdImage {
            id: speed_box
            elementName: "speed-box"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width: parent.width
            height: elementBounds.height*width/elementBounds.width
            anchors.leftMargin: 5

            StripNum {
                id: speed_num1
                anchors.fill: parent
                anchors.topMargin: parent.height*0.05
                anchors.bottomMargin: anchors.topMargin
                anchors.rightMargin: (parent.width)*0.22
                anchors.leftMargin: (parent.width)*0.5
                value: speed_window.speed_value
            }
            StripNum {
                anchors.fill: parent
                anchors.topMargin: parent.height*0.25
                anchors.bottomMargin: anchors.topMargin
                anchors.rightMargin: speed_num1.x
                anchors.leftMargin: (parent.width)*0.1
                numScale: 1.2
                showZero: false
                divider: 10
                value: speed_window.speed_value
            }
            ToolTipArea {text: f_airspeed.descr}
        }


    }
    PfdImage {
        id: speed_triangle
        elementName: "speed-triangle"
        //smooth: ui.antialiasing
        width: elementBounds.width*speed_window.strip_scale
        height: elementBounds.height*speed_window.strip_scale
        anchors.left: speed_window.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: apx.limit(speed_window.num2scaleHeight * (f_airspeed.value - f_speed.value),-speed_window.height/2,speed_window.height/2)
        Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        Text {
            visible: Math.abs(f_airspeed.value - f_speed.value)>3 || f_speed.value<10
            text: f_speed.value.toFixed()
            color: "white"
            anchors.left: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 4
            font.pixelSize: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
        ToolTipArea {text: f_speed.descr}
    }

}
