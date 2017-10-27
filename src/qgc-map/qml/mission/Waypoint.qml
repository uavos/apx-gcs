import QtQuick 2.2
import QtGraphicalEffects 1.0
import "."
import "../"
import "../components"
import com.uavos.map 1.0


MapObject {
    id: waypointItem
    mItem: model.modelData //mission.waypoints[index]
    parent: missionItem

    property var f_type: mItem.child("type")
    property var f_altitude: mItem.child("altitude")
    property var f_actions: mItem.child("actions")

    property int type: f_type?m.f_type.value:0
    property int altitude: f_altitude?m.f_altitude.value:0

    color: Style.cYellow
    textColor: "black"

    property bool showDetails: mItem.DW===0 || mItem.DW*mapProvider.metersToX(gy)*map.constSceneXY>height

    FastBlur {
        id: courseArrow
        x: -width/2+parent.width/2
        y: width+parent.height/2
        width: waypointItem.height/mapProvider.itemScaleFactor//*1.5
        height: width
        transform: Rotation {
            origin.x: courseArrow.width/2
            origin.y: -courseArrow.width
            axis.z: 1
            angle: mItem.course
        }
        radius: 4
        opacity: 0.6
        visible: showDetails
        source: ColorOverlay {
            width: courseArrow.height
            height: width
            source: Image {
                width: courseArrow.height
                height: width
                source: "/icons/ionicons/android-navigate.svg"
            }
            color: path.color //"white"
        }
    }

    //right area info
    Column{
        z: -10
        spacing: 1
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: (parent.width+width/2)/map.tmpScale
        scale: 1/map.tmpScale
        MapText {
            textColor: "white"
            color: Style.cGreen
            text: altitude.toFixed()+"m"
            opacity: (!dragging) && (hover||selected)?1:0
            scale: 1
        }
    }
    //bottom area info (actions)
    MapText {
        z: -99
        anchors.centerIn: parent
        anchors.verticalCenterOffset: (parent.height)/map.tmpScale
        visible: f_actions.text!=="off" && opacity>0
        textColor: "white"
        color: Style.cRed
        text: f_actions.text
        opacity: (!dragging)?((hover||selected)?1:((mapProvider.level>13 && showDetails)?0.6:0)):0
    }
    //top area info - DME on dragging
    Column{
        z: -10
        spacing: 1
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -(parent.height+height)/map.tmpScale
        scale: 1/map.tmpScale
        MapText {
            scale: 1
            textColor: "white"
            color: Style.cNormal
            text: app.distanceToString(mItem.DW)+"/"+app.distanceToString(mItem.DT)
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
        }
        MapText {
            scale: 1
            textColor: "white"
            color: Style.cNormal
            text: app.timeToString(mItem.time)+"/"+app.timeToString(mItem.ETA)
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
        }
    }


    QmlMapPath {
        id: path
        z: -100
        parent: missionItem
        provider: mapProvider
        lineWidth: index==0?2:4
        property color cReachable: mItem.warning?Style.cLineYellow:type===0?Style.cLineCyan:Style.cLineBlue
        color: index==0?Style.cLineGreen: mItem.reachable?cReachable:Style.cLineRed
        opacity: 0.6
        path: mItem.path
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
    }
}
