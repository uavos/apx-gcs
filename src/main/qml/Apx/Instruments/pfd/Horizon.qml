import QtQuick 2.2
import APX.Vehicles 1.0
import "../common"
import "."
//import QtGraphicalEffects 1.0



Item {
    id: horizon
    anchors.fill: parent
    anchors.centerIn: parent
    property bool showHeading: true
    property double margin_left
    property double margin_right
    property double center_shift: parent.width*(margin_left-margin_right)/2
    transform: Translate {x: center_shift}

    property double sf: 3   //2.8
    property double anumation_duration: 100

    property int diagonal: Math.sqrt(Math.pow(height, 2) + Math.pow(width, 2)) + 50
    property double pitchDeg2img:
        ((svgRenderer.elementBounds(pfdImageUrl, "pitch-5").y -
          svgRenderer.elementBounds(pfdImageUrl, "pitch-15").y)/10.0) *
        horizon_scale.height/horizon_scale.elementBounds.height
    property double rollDeg2img: width*0.4/45

    opacity: ui.effects?((apx.datalink.valid && !(apx.vehicles.current.streamType===Vehicle.XPDR||apx.vehicles.current.streamType===Vehicle.TELEMETRY))?0.7:1):1

    Item{
        id: horizon_bg
        anchors.fill: parent
        rotation: apx.angle(-m.roll.value)
        Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }

        PfdImage {
            id: horizon_image
            elementName: "horizon-bg"
            opacity: 1
            //smooth: ui.antialiasing
            fillMode: Image.PreserveAspectFit
            width: diagonal*sf
            height: width
            property double value: m.pitch.value
            anchors.centerIn: parent
            anchors.verticalCenterOffset: value*pitchDeg2img
            Behavior on value { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration } }
        }

        Rectangle {
            clip: true
            anchors.centerIn: parent
            anchors.fill: parent
            anchors.topMargin: (parent.height-roll_scale_image.height)*1.4
            anchors.bottomMargin: -parent.height
            border.width: 0
            color: "transparent" //"#50000000"
            PfdImage {
                id: horizon_scale
                elementName: "horizon-scale"
                //smooth: ui.antialiasing
                opacity: ui.effects?0.5:1
                fillMode: Image.PreserveAspectFit
                width: parent.width*0.3
                height: elementBounds.height*width/elementBounds.width
                anchors.centerIn: parent
                anchors.verticalCenterOffset: (parent.anchors.bottomMargin-parent.anchors.topMargin)/2+horizon_image.anchors.verticalCenterOffset
            }
        }

    }

    Rectangle {
        id: center_window
        clip: true
        color: "transparent"
        border.width: 0
        anchors.fill: parent
        anchors.centerIn: parent
        anchors.leftMargin: parent.width*horizon.margin_left-horizon.center_shift
        anchors.rightMargin: parent.width*horizon.margin_right+horizon.center_shift

        Heading {
            id: heading_window
            visible: showHeading
        }

        PfdImage {
            id: roll_scale_image
            elementName: "roll-scale"
            //smooth: ui.antialiasing
            border: 1
            property double sf: parent.width*0.5/elementBounds.width
            fillMode: Image.PreserveAspectFit
            width: elementBounds.width*sf
            height: elementBounds.height*sf
            anchors.fill: parent
            anchors.topMargin: (heading_window.visible?heading_window.height:0)+3
            anchors.bottomMargin: anchors.topMargin
            anchors.centerIn: parent

            rotation: horizon_bg.rotation
        }

        PfdImage {
            id: sideslip_fixed_image
            elementName: "sideslip-fixed"
            //smooth: ui.antialiasing
            fillMode: Image.PreserveAspectFit
            width: roll_scale_image.width
            height: roll_scale_image.height
            anchors.fill: parent
            anchors.topMargin: roll_scale_image.anchors.topMargin
            anchors.bottomMargin: anchors.topMargin
        }

        PfdImage {
            id: sideslip_moving_image
            elementName: "sideslip-moving"
            //smooth: ui.antialiasing
            rotation: -m.slip.value
            fillMode: Image.PreserveAspectFit
            width: roll_scale_image.width
            height: roll_scale_image.height
            anchors.fill: parent
            anchors.topMargin: roll_scale_image.anchors.topMargin
            anchors.bottomMargin: anchors.topMargin
            Behavior on rotation { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        }

        Rectangle {
            id: rc_ptr
            property bool active: m.rc_roll.value!==0 || m.rc_pitch.value!==0
            width: rc_ctr.ptr_size+5
            height: width
            radius: width*0.5
            color: "#80101010"
            visible: active
            x: (m.rc_roll.value+1)/2*rc_ctr.width-width/2
            y: rc_ctr.anchors.topMargin+(m.rc_pitch.value+1)/2*rc_ctr.height-height/2
            Behavior on x { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration*0.8} }
            Behavior on y { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration*0.8} }

        }

        ControlArea {
            id: rc_ctr
            anchors.topMargin: showHeading?heading_window.height:0
            anchors.bottomMargin: anchors.topMargin
            anchors.fill: parent
            //anchors.verticalCenterOffset: -anchors.topMargin
            //anchors.centerIn: parent
            mvarX: m.rc_roll
            mvarY: m.rc_pitch
        }

    }


    //Flight directors
    Rectangle {
        //ailerons
        antialiasing: ui.smooth
        width: fd_roll.width*0.7
        height: fd_roll.height*0.25
        color: "#990099"
        anchors.centerIn: parent
        //anchors.horizontalCenterOffset: apx.limit(fd_roll.pos+(m.ctr_ailerons.value*fd_roll.height*0.5),-parent.width*0.2,parent.width*0.2)
        anchors.horizontalCenterOffset: apx.limit((m.ctr_ailerons.value*fd_roll.height*0.5),-parent.width*0.2,parent.width*0.2)
        Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
    }
    Rectangle {
        //elevator
        antialiasing: ui.smooth
        width: fd_pitch.width*0.25
        height: fd_pitch.height*0.7
        color: "#990099"
        anchors.centerIn: parent
        //anchors.verticalCenterOffset: apx.limit(fd_pitch.pos-(m.ctr_elevator.value*fd_pitch.width*0.5),-parent.height*0.4,parent.height*0.4)
        anchors.verticalCenterOffset: apx.limit(-(m.ctr_elevator.value*fd_pitch.width*0.5),-parent.height*0.4,parent.height*0.4)
        Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
    }
    Rectangle {
        //roll
        id: fd_roll
        antialiasing: ui.smooth
        width: apx.limit(diagonal*0.006,2,8)
        height: parent.width*0.2
        color: "magenta"
        property double pos: -(m.roll.value-m.cmd_roll.value)*rollDeg2img
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: apx.limit(pos,-parent.width*0.2,parent.width*0.2)
        Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
    }
    Rectangle {
        //pitch
        id: fd_pitch
        antialiasing: ui.smooth
        height: apx.limit(diagonal*0.006,2,8)
        width: parent.width*0.2
        color: "magenta"
        property double pos: (m.pitch.value-m.cmd_pitch.value)*pitchDeg2img
        anchors.centerIn: parent
        anchors.verticalCenterOffset: apx.limit(pos,-parent.height*0.4,parent.height*0.4)
        Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
    }

    /*PfdImage {
        id: fd_pitch_image
        elementName: "fd-pitch"
        smooth: ui.antialiasing
        fillMode: Image.PreserveAspectFit
        width: parent.width*0.2
        height: width
        anchors.centerIn: parent
        anchors.verticalCenterOffset: apx.limit((m.pitch.value-m.cmd_pitch.value)*pitchDeg2img,-parent.height*0.4,parent.height*0.4)
        Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
    }

    PfdImage {
        id: fd_roll_image
        elementName: "fd-roll"
        smooth: ui.antialiasing
        fillMode: Image.PreserveAspectFit
        width: fd_pitch_image.height
        height: fd_pitch_image.width
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: apx.limit(-(m.roll.value-m.cmd_roll.value)*rollDeg2img,-parent.width*0.2,parent.width*0.2)
        Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
    }*/

    //Center sign
    PfdImage {
        id: horizon_center
        elementName: "horizon-center"
        //smooth: ui.antialiasing
        fillMode: Image.PreserveAspectFit
        anchors.centerIn: parent
        width: parent.width*0.2
        height: width
    }

}
