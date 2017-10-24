import QtQuick 2.2
import "../components"

ControlArea {
    mvar: cmd_airspeed   //ControlArea
    span: 10
    min:0
    max: 100
    step: 1
    stepDrag: step*0.5
    stepLimit: 1
    fixedPoint: true
    doWheel: false

    //instrument item
    property double anumation_duration: app.settings.smooth.value?200:0
    //anchors.fill: none
    //anchors.verticalCenter: parent.verticalCenter
    //anchors.verticalCenterOffset: parent.anchors.verticalCenterOffset

    /*function adj(v) {
        v=parseInt(v)
        v=cmd_airspeed.value+v*1
        if(v<0)v=0;
        cmd_airspeed.setValue(v);
    }
    onWheel: adj(wheel.angleDelta.y>0?1:-1)
    onClicked: adj(out_yv<0?1:-1)
    Timer {
        interval: 500
        repeat: true
        running: parent.drag.active
        onTriggered: {
            if(parent.out_y!=0){
                adj((parent.out_y<0)?1:-1);
            }
        }
    }*/

    Rectangle {
        id: speed_window
        //color: "#20000000"
        color: "transparent"
        border.width: 0
        clip: true
        anchors.fill: parent
        anchors.verticalCenter: parent.verticalCenter

        property double strip_width: 0.3

        property real speed_value: airspeed.value   //pitch.value //Math.abs(pitch.value)
        Behavior on speed_value { PropertyAnimation {duration: anumation_duration} }

        property double strip_scale: width*strip_width/svgRenderer.elementBounds("pfd/pfd.svg", "speed-scale").width
        property double num2scaleHeight:
            svgRenderer.elementBounds("pfd/pfd.svg", "speed-scale").height * strip_scale /10

        Item {
            id: speed_scale
            anchors.verticalCenter: parent.verticalCenter
            //anchors.rightMargin: 1
            anchors.right: parent.right
            width: parent.width*speed_window.strip_width
            //height: parent.height

            property int item_cnt: Math.floor(speed_window.height/speed_window.num2scaleHeight*10/2)*2+2
            Repeater {
                model: 8    //speed_scale.item_cnt
                PfdImage {
                    id: speed_scale_image
                    smooth: true
                    elementName: "speed-scale"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 1
                    anchors.right: parent.right
                    anchors.verticalCenterOffset: (index-2)*height + height/10 * (speed_window.speed_value-Math.floor(speed_window.speed_value/1)*1)
                    width: parent.width
                    height: elementBounds.height*speed_window.strip_scale
                    //speed_scale.scale_size: height
                }
            }
        }

        Item {
            id: scale_numbers
            smooth: true
            //anchors.fill: parent

            anchors.right: parent.right
            anchors.rightMargin: speed_window.width*speed_window.strip_width
            anchors.verticalCenter: parent.verticalCenter
            //anchors.verticalCenterOffset: speed_window.num2scaleHeight*(speed_window.speed_value-Math.floor(speed_window.speed_value))

            property int item_cnt: Math.floor(speed_window.height/speed_window.num2scaleHeight/2)*2+2
            property int topNumber: Math.floor(speed_window.speed_value)+item_cnt/2
            Repeater {
                model: scale_numbers.item_cnt
                Item {
                    //property double posy: (index-scale_numbers.item_cnt/2+(speed_window.speed_value-Math.floor(speed_window.speed_value)))*speed_window.num2scaleHeight
                    //smooth: true
                    //height: speed_window.num2scaleHeight  //speed_scale.scale_size / 10
                    width: scale_numbers.width
                    //anchors.fill: parent
                    //anchors.topMargin: (index-2)*speed_window.num2scaleHeight
                    //anchors.right: parent.right
                    //smooth: true
                    transform: [
                        Scale { yScale: 1.01 }, //force render as img
                        Translate { y: (index-scale_numbers.item_cnt/2+(speed_window.speed_value-Math.floor(speed_window.speed_value)))*speed_window.num2scaleHeight }
                    ]
                    //y: (index-scale_numbers.item_cnt/2)*speed_window.num2scaleHeight

                    Text {
                        property int num: scale_numbers.topNumber - index
                        smooth: true
                        text: num
                        visible: num>=0
                        //render as image
                        style: Text.Raised
                        styleColor: "transparent"

                        color: "#A0FFFFFF"
                        //color: "white"
                        font.family: font_condenced
                        font.bold: true
                        font.pixelSize: speed_window.num2scaleHeight / 1.5
                        //anchors.fill: parent
                        //anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        //anchors.topMargin: (index-2)*speed_window.num2scaleHeight
                    }
                }
            }
        }

        PfdImage {
            id: speed_waypoint
            elementName: "speed-waypoint"
            smooth: true
            //visible: cmd_airspeed.value !== 0.0
            width: elementBounds.width*height/elementBounds.height  //speed_window.strip_scale
            height: speed_box.height    //elementBounds.height*speed_window.strip_scale

            anchors.right: speed_scale.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: sys.limit(speed_window.num2scaleHeight * (airspeed.value - cmd_airspeed.value),-parent.height/2,parent.height/2)
            Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
            ToolTipArea {text: cmd_airspeed.descr}
        }

        PfdImage {
            id: speed_box
            elementName: "speed-box"
            smooth: true
            //clip: true
            //fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width: parent.width //elementBounds.width*speed_window.scaleFactor
            height: elementBounds.height*width/elementBounds.width  //speed_window.scaleFactor
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
                value: speed_window.speed_value //+(speed_window.speed_value>=0?0.5:-0.5)
            }
            ToolTipArea {text: airspeed.descr}
        }


    }
    PfdImage {
        id: speed_triangle
        elementName: "speed-triangle"
        smooth: true
        width: elementBounds.width*speed_window.strip_scale
        height: elementBounds.height*speed_window.strip_scale
        anchors.left: speed_window.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: sys.limit(speed_window.num2scaleHeight * (airspeed.value - gSpeed.value),-speed_window.height/2,speed_window.height/2)
        Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
        Text {
            visible: Math.abs(airspeed.value - gSpeed.value)>3 || gSpeed.value<10
            text: gSpeed.value.toFixed()
            color: "white"
            anchors.left: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 4
            font.pixelSize: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
        ToolTipArea {text: gSpeed.descr}
    }

}
