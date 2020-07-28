import QtQuick 2.2
import "../common"

ControlArea {

    readonly property var f_vspeed: mandala.est.pos.vspeed
    readonly property real m_vspeed: f_vspeed.value

    readonly property var f_cmd_vspeed: mandala.cmd.pos.vspeed
    readonly property real m_cmd_vspeed: f_cmd_vspeed.value

    //readonly property var f_vd: mandala.est.pos.vspeed
    //readonly property real m_vd: f_vd.value

    readonly property var f_vse: mandala.est.air.vse
    readonly property real m_vse: f_vse.value


    mvar: mandala.cmd.rc.thr   //ControlArea
    //speed: 0.8

    //instrument item
    id: vsi_window
    property double anumation_duration: 200
    anchors.verticalCenter: parent.verticalCenter

    property double scaleFactor: vsi_scale.height/svgRenderer.elementBounds(pfdImageUrl, "vsi-scale").height
    function get_deg(v){
        var max=24;
        var sign=v>=0?1:-1;
        v=Math.abs(v);
        if(Math.abs(v)>10)return sign*max;
        if(Math.abs(v)>5)return sign*(16+(v-5)/5*8);
        if(Math.abs(v)>1)return sign*(8+(v-1)/4*8);
        return sign*(v*8);
    }

    PfdImage {
        id: vsi_scale
        elementName: "vsi-scale"
        //smooth: ui.antialiasing
        width: elementBounds.width*vsi_window.scaleFactor  //parent.width*0.8
        height: apx.limit(1.2*parent.width*elementBounds.height/elementBounds.width,0,parent.height*0.8)  //elementBounds.height*vsi_window.scaleFactor
        //anchors.left: parent.left
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -parent.width*0.3
        anchors.verticalCenter: parent.verticalCenter
    }

    PfdImage {
        id: vsi_arrow
        elementName: "vsi-arrow"
        //smooth: ui.antialiasing
        border: 1
        width: elementBounds.width*vsi_window.scaleFactor+2
        height: elementBounds.height*vsi_window.scaleFactor+2
        anchors.left: vsi_scale.horizontalCenter
        anchors.verticalCenter: vsi_scale.verticalCenter
        transform: Rotation{
            origin.x: vsi_arrow.width
            origin.y: vsi_arrow.height/2
            angle: get_deg(m_vspeed)
            Behavior on angle { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        }
        ToolTipArea {text: f_vspeed.descr}
    }

    /*PfdImage {
        id: vsi_gps
        elementName: "vsi-gps"
        //smooth: ui.antialiasing
        width: elementBounds.width*vsi_window.scaleFactor
        height: elementBounds.height*vsi_window.scaleFactor
        anchors.left: vsi_scale.horizontalCenter
        anchors.verticalCenter: vsi_scale.verticalCenter
        transform: Rotation{
            origin.x: vsi_gps.width
            origin.y: vsi_gps.height/2
            angle: get_deg(m_vd)
            Behavior on angle { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        }
        ToolTipArea {text: f_vd.descr}
    }*/

    PfdImage {
        id: vsi_triangle
        elementName: "vsi-triangle"
        //smooth: ui.antialiasing
        width: elementBounds.width*vsi_window.scaleFactor
        height: elementBounds.height*vsi_window.scaleFactor
        anchors.left: vsi_scale.horizontalCenter
        anchors.verticalCenter: vsi_scale.verticalCenter
        transform: Rotation{
            origin.x: vsi_triangle.width
            origin.y: vsi_triangle.height/2
            angle: get_deg(m_vse+m_vspeed)
            Behavior on angle { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        }
        ToolTipArea {text: f_vse.descr}
    }

    PfdImage {
        id: vsi_waypoint
        elementName: "vsi-waypoint"
        //smooth: ui.antialiasing
        border: 1
        width: elementBounds.width*vsi_window.scaleFactor+2
        height: elementBounds.height*vsi_window.scaleFactor+2
        anchors.left: vsi_scale.horizontalCenter
        anchors.verticalCenter: vsi_scale.verticalCenter
        transform: Rotation{
            origin.x: vsi_waypoint.width
            origin.y: vsi_waypoint.height/2
            angle: get_deg(m_cmd_vspeed)
            Behavior on angle { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        }
        ToolTipArea {text: f_cmd_vspeed.descr}
    }

    Text {
        id: vsi_text_low
        visible: m_vspeed<-0.5
        text: m_vspeed.toFixed(1)
        color: "white"
        anchors.horizontalCenter: vsi_scale.right
        anchors.horizontalCenterOffset: -vsi_scale.width*0.1
        anchors.top: vsi_scale.bottom
        anchors.topMargin: -12
        font.pixelSize: vsi_scale.height*0.1 //Math.min(parent.width*0.5,parent.height*0.1)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: font_narrow
    }

    Text {
        visible: m_vspeed>0.5
        text: "+"+m_vspeed.toFixed(1)
        color: "white"
        anchors.horizontalCenter: vsi_scale.right
        anchors.bottom: vsi_scale.top
        anchors.bottomMargin: -12
        anchors.horizontalCenterOffset: vsi_text_low.anchors.horizontalCenterOffset
        font: vsi_text_low.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

}
