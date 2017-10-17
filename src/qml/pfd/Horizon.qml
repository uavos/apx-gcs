import QtQuick 2.2
import "../components"
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
    property double anumation_duration: app.settings.smooth.value?100:0

    property int diagonal: Math.sqrt(Math.pow(height, 2) + Math.pow(width, 2)) + 50
    property double pitchDeg2img:
        ((svgRenderer.elementBounds("pfd/pfd.svg", "pitch-5").y -
          svgRenderer.elementBounds("pfd/pfd.svg", "pitch-15").y)/10.0) *
        horizon_scale.height/horizon_scale.elementBounds.height
    property double rollDeg2img: width*0.4/45

    opacity: app.settings.smooth.value?((mandala.dlcnt>0 && !(mandala.xpdrData||mandala.dlinkData))?0.7:1):1

    Item{
        id: horizon_bg
        anchors.fill: parent
        rotation: mandala.angle(-roll.value)
        Behavior on rotation { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }

        PfdImage {
            id: horizon_image
            elementName: "horizon-bg"
            opacity: 1
            smooth: true
            fillMode: Image.PreserveAspectFit
            width: diagonal*sf
            height: width
            property double value: pitch.value
            anchors.centerIn: parent
            anchors.verticalCenterOffset: value*pitchDeg2img
            Behavior on value { PropertyAnimation {duration: anumation_duration} }
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
                smooth: true
                opacity: 0.5
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
            smooth: true
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
            smooth: true
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
            smooth: true
            rotation: -slip.value
            fillMode: Image.PreserveAspectFit
            width: roll_scale_image.width
            height: roll_scale_image.height
            anchors.fill: parent
            anchors.topMargin: roll_scale_image.anchors.topMargin
            anchors.bottomMargin: anchors.topMargin
            Behavior on rotation { PropertyAnimation {duration: anumation_duration} }
        }

        Rectangle {
            id: rc_ptr
            property bool active: rc_roll.value!==0 || rc_pitch.value!==0
            width: rc_ctr.ptr_size+5
            height: width
            radius: width*0.5
            color: "#80101010"
            visible: active
            x: (rc_roll.value+1)/2*rc_ctr.width-width/2
            y: rc_ctr.anchors.topMargin+(rc_pitch.value+1)/2*rc_ctr.height-height/2
            Behavior on x { PropertyAnimation {duration: anumation_duration*0.8} }
            Behavior on y { PropertyAnimation {duration: anumation_duration*0.8} }

        }

        ControlArea {
            id: rc_ctr
            anchors.topMargin: showHeading?heading_window.height:0
            anchors.bottomMargin: anchors.topMargin
            anchors.fill: parent
            //anchors.verticalCenterOffset: -anchors.topMargin
            //anchors.centerIn: parent
            mvarX:  rc_roll
            mvarY:  rc_pitch
        }

    }


    //Flight directors
    Rectangle {
        //ailerons
        antialiasing: app.settings.smooth.value
        width: fd_roll.width*0.7
        height: fd_roll.height*0.25
        color: "#990099"
        anchors.centerIn: parent
        //anchors.horizontalCenterOffset: mandala.limit(fd_roll.pos+(ctr_ailerons.value*fd_roll.height*0.5),-parent.width*0.2,parent.width*0.2)
        anchors.horizontalCenterOffset: mandala.limit((ctr_ailerons.value*fd_roll.height*0.5),-parent.width*0.2,parent.width*0.2)
        Behavior on anchors.horizontalCenterOffset { PropertyAnimation {duration: anumation_duration} }
    }
    Rectangle {
        //elevator
        antialiasing: app.settings.smooth.value
        width: fd_pitch.width*0.25
        height: fd_pitch.height*0.7
        color: "#990099"
        anchors.centerIn: parent
        //anchors.verticalCenterOffset: mandala.limit(fd_pitch.pos-(ctr_elevator.value*fd_pitch.width*0.5),-parent.height*0.4,parent.height*0.4)
        anchors.verticalCenterOffset: mandala.limit(-(ctr_elevator.value*fd_pitch.width*0.5),-parent.height*0.4,parent.height*0.4)
        Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
    }
    Rectangle {
        //roll
        id: fd_roll
        antialiasing: app.settings.smooth.value
        width: mandala.limit(diagonal*0.006,2,8)
        height: parent.width*0.2
        color: "magenta"
        property double pos: -(roll.value-cmd_roll.value)*rollDeg2img
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: mandala.limit(pos,-parent.width*0.2,parent.width*0.2)
        Behavior on anchors.horizontalCenterOffset { PropertyAnimation {duration: anumation_duration} }
    }
    Rectangle {
        //pitch
        id: fd_pitch
        antialiasing: app.settings.smooth.value
        height: mandala.limit(diagonal*0.006,2,8)
        width: parent.width*0.2
        color: "magenta"
        property double pos: (pitch.value-cmd_pitch.value)*pitchDeg2img
        anchors.centerIn: parent
        anchors.verticalCenterOffset: mandala.limit(pos,-parent.height*0.4,parent.height*0.4)
        Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
    }

    /*PfdImage {
        id: fd_pitch_image
        elementName: "fd-pitch"
        smooth: true
        fillMode: Image.PreserveAspectFit
        width: parent.width*0.2
        height: width
        anchors.centerIn: parent
        anchors.verticalCenterOffset: mandala.limit((pitch.value-cmd_pitch.value)*pitchDeg2img,-parent.height*0.4,parent.height*0.4)
        Behavior on anchors.verticalCenterOffset { PropertyAnimation {duration: anumation_duration} }
    }

    PfdImage {
        id: fd_roll_image
        elementName: "fd-roll"
        smooth: true
        fillMode: Image.PreserveAspectFit
        width: fd_pitch_image.height
        height: fd_pitch_image.width
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: mandala.limit(-(roll.value-cmd_roll.value)*rollDeg2img,-parent.width*0.2,parent.width*0.2)
        Behavior on anchors.horizontalCenterOffset { PropertyAnimation {duration: anumation_duration} }
    }*/

    //Center sign
    PfdImage {
        id: horizon_center
        elementName: "horizon-center"
        smooth: true
        fillMode: Image.PreserveAspectFit
        anchors.centerIn: parent
        width: parent.width*0.2
        height: width
    }

}
