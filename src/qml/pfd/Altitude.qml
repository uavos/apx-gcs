import QtQuick 2.6
import QtQuick.Controls 2.1
//import QtQuick 2.2
//import QtQuick.Controls 1.1
import "../components"

ControlArea {
    mvar: cmd_altitude   //ControlArea
    span: 20
    min:0
    max: 50000
    fixedPoint: true
    step: (cmd_altitude.value<100)?1:cmd_altitude.value<800?10:100
    stepDrag: step*2
    stepWheel: step*0.0001
    stepLimit: 100

    //instrument item
    property double anumation_duration: app.settings.smooth.value?200:0
    anchors.fill: parent
    //anchors.verticalCenter: parent.verticalCenter

    /*function adj(v) {
        v=parseInt(v)
        if(cmd_altitude.value<100) cmd_altitude.setValue(cmd_altitude.value+v*1);
        else if(cmd_altitude.value<800) cmd_altitude.setValue(((cmd_altitude.value+(v*10))/10).toFixed()*10);
        else cmd_altitude.setValue(((cmd_altitude.value+(v*100))/100).toFixed()*100);
    }
    onWheel: adj((wheel.angleDelta.y>0)?1:-1)
    onClicked: adj(out_yv<0?+1:-1)
    Timer {
        interval: 500
        repeat: true
        running: parent.drag.active
        onTriggered: {
            if(parent.out_y!=0){
                adj((parent.out_y<0)?1:-1)
            }
            interval=500-Math.abs(parent.out_y)*400
        }
    }*/

    Rectangle {
        id: altitude_window
        //color: "#20000000"
        color: "transparent"
        border.width: 0
        clip: true
        anchors.fill: parent
        //anchors.verticalCenter: parent.verticalCenter

        property double strip_width: 0.2
        property double strip_factor: 10

        property real altitude_value : altitude.value   //pitch.value //Math.abs(pitch.value)
        Behavior on altitude_value { PropertyAnimation {duration: anumation_duration} }

        property variant scale_bounds: svgRenderer.elementBounds("pfd/pfd.svg", "altitude-scale")
        property double strip_scale: (width>0?width:10)*strip_width/scale_bounds.width
        property double num2scaleHeight: scale_bounds.height * strip_scale /10

        Item {
            id: altitude_scale
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 1
            anchors.left: parent.left
            width: parent.width*altitude_window.strip_width

            property int item_cnt: Math.floor(altitude_window.height/altitude_window.num2scaleHeight*10/2)*2+2
            Repeater {
                model: 8    //altitude_scale.item_cnt
                PfdImage {
                    id: altitude_scale_image
                    smooth: true
                    elementName: "altitude-scale"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 1
                    anchors.right: parent.right
                    anchors.verticalCenterOffset: (index-2)*height + height/10 * (altitude_window.altitude_value/altitude_window.strip_factor-Math.floor(altitude_window.altitude_value/altitude_window.strip_factor))
                    width: parent.width
                    height: elementBounds.height*altitude_window.strip_scale
                    //altitude_scale.scale_size: height
                }
            }
        }

        Item {
            id: scale_numbers
            smooth: true
            //anchors.fill: parent

            anchors.left: parent.left
            anchors.leftMargin: altitude_window.width*altitude_window.strip_width
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            property int item_cnt: Math.floor(altitude_window.height/altitude_window.num2scaleHeight/2)*2+2
            property int topNumber: Math.floor(altitude_window.altitude_value/altitude_window.strip_factor)+item_cnt/2
            Repeater {
                model: scale_numbers.item_cnt
                Item {
                    width: scale_numbers.width
                    transform: [
                        Scale { yScale: 1.01 }, //force render as img
                        Translate { y: (index-scale_numbers.item_cnt/2+(altitude_window.altitude_value/altitude_window.strip_factor-Math.floor(altitude_window.altitude_value/altitude_window.strip_factor)))*altitude_window.num2scaleHeight }
                    ]

                    Text {
                        property int num: scale_numbers.topNumber - index
                        smooth: true
                        text: num*altitude_window.strip_factor
                        visible: num>=0
                        //render as image
                        style: Text.Raised
                        styleColor: "transparent"

                        color: "#A0FFFFFF"
                        font.family: font_condenced
                        font.bold: true
                        font.pixelSize: altitude_window.num2scaleHeight / 1.5
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        PfdImage {
            id: altitude_waypoint
            elementName: "altitude-waypoint"
            smooth: true
            //visible: cmd_airaltitude.value !== 0.0
            width: elementBounds.width*height/elementBounds.height  //altitude_window.strip_scale
            height: altitude_box.height    //elementBounds.height*altitude_window.strip_scale

            anchors.left: altitude_scale.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: sys.limit(altitude_window.num2scaleHeight * (altitude.value - cmd_altitude.value)/altitude_window.strip_factor,-parent.height/2,parent.height/2)
            Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
            ToolTipArea {text: cmd_altitude.descr}
        }

        PfdImage {
            id: altitude_box
            elementName: "altitude-box"
            smooth: true
            clip: true
            //fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width: parent.width //elementBounds.width*altitude_window.scaleFactor
            height: elementBounds.height*width/elementBounds.width  //altitude_window.scaleFactor
            anchors.leftMargin: 5

            StripNum {
                anchors.fill: parent
                anchors.topMargin: parent.height*0.06
                anchors.bottomMargin: anchors.topMargin
                anchors.leftMargin: parent.width*0.85
                anchors.rightMargin: parent.width*0.03
                //numScale: 0.35
                numGapScale: 1.2 //0.9
                value: altitude_window.altitude_value
            }
            Row {
                spacing: 0
                layoutDirection: Qt.RightToLeft
                anchors.fill: parent
                anchors.topMargin: parent.height*0.25
                anchors.bottomMargin: parent.height*0.2
                anchors.leftMargin: parent.width*0.1
                anchors.rightMargin: parent.width*0.15
                Repeater {
                    model: 4
                    StripNum {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width*0.2
                        numScale: 1.2
                        color: (value<divider)?"gray":"white"
                        divider: Math.pow(10,index+1)
                        value: altitude_window.altitude_value
                    }
                }
            }
            ToolTipArea {text: altitude.descr}
        }

    }

    PfdImage {
        id: altitude_triangle
        elementName: "altitude-triangle"
        smooth: true
        visible: app.settings.test.value || app.datalink.valid
        width: elementBounds.width*altitude_window.strip_scale
        height: elementBounds.height*altitude_window.strip_scale
        anchors.right: altitude_window.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: sys.limit(altitude_window.num2scaleHeight * (altitude.value - (gps_hmsl.value-home_hmsl.value))/altitude_window.strip_factor,-altitude_window.height/2,altitude_window.height/2)
        Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
        Text {
            visible: Math.abs(altitude.value - (gps_hmsl.value-home_hmsl.value))>10
            text: (gps_hmsl.value-home_hmsl.value).toFixed()
            color: "white"
            anchors.right: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 4
            font.pixelSize: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
        ToolTipArea {text: gps_hmsl.descr}
    }

    PfdImage {
        id: agl_image
        elementName: "agl"
        smooth: true
        visible: status_agl.value>0
        width: elementBounds.width*altitude_window.strip_scale
        height: elementBounds.height*altitude_window.strip_scale
        anchors.right: altitude_window.left
        //anchors.rightMargin: 4
        anchors.top: parent.verticalCenter
        anchors.topMargin: sys.limit(5*altitude_window.num2scaleHeight * (agl.value)/altitude_window.strip_factor,-altitude_window.height/2,altitude_window.height/2)
        Behavior on anchors.topMargin { PropertyAnimation {duration: anumation_duration} }
        Text {
            visible: agl.value>0
            text: agl.value.toFixed(agl.value>10?0:1)
            color: "yellow"
            anchors.right: parent.left
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
        }
        ToolTipArea {text: agl.descr}
    }

}
