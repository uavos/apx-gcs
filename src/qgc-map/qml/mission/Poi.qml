import QtQuick 2.2
import "."
import "../"
import "../components"
import com.uavos.map 1.0


MapObject {
    id: poiItem
    mItem: model.modelData
    parent: missionItem

    property var f_HMSL: mItem.child("HMSL")
    property var f_turnR: mItem?mItem.child("turnR"):0
    property var f_loops: mItem.child("loops")
    property var f_time: mItem.child("time")

    property int turnR: f_turnR?m.f_turnR.value:0
    property int hmsl: f_HMSL?m.f_HMSL.value:0
    property int loops: f_loops?m.f_loops.value:0
    property int time: f_time?m.f_time.value:0

    //property bool current: m.piidx.value === index
    //property bool watching: m.mode.value === mode_STBY

    color: Style.cBlue
    textColor: "white"

    property bool showDetails: mapProvider.level>13

    //top area info
    MapText {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: (parent.width/2+width/2)/map.tmpScale
        visible: hmsl!==0 && opacity>0
        textColor: "white"
        color: Style.cGreen
        text: hmsl+"m"
        opacity: (!dragging)?((hover||selected)?1:(showDetails?0.6:0)):0
    }
    //bottom area info
    MapText {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: (parent.height)/map.tmpScale
        visible: (loops>0 || time>0) && opacity>0
        textColor: "white"
        color: Style.cNormal
        text: (loops>0?"L"+loops:"")+
              (time>0?"T"+f_time.text:"")
        opacity: (!dragging)?((hover||selected)?1:(showDetails?0.6:0)):0
    }


    //circle radius handle
    MapObject {
        id: turnHandle
        parent: missionItem
        parentObject: poiItem
        z: poiItem.z-1 //above all
        lat: poiItem.mItem.turnPoint.x
        lon: poiItem.mItem.turnPoint.y
        color: "white"
        textColor: "black"
        property int v: turnR.toFixed()
        opacity: (poiItem.hover || poiItem.selected)?(v>0?0.8:0.5):0
        visible: opacity>0
        text: v>0?(v+"m"):"H----"
        onObjectMoved: {
            snapFromModel(mlat,mlon);
            poiItem.mItem.turnPoint=Qt.point(mlat,mlon);
        }
    }
    QmlMapPath {
        z: -100
        parent: missionItem
        provider: mapProvider
        lineWidth: 1
        color: "blue"
        path: mItem.pathTurn
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
        opacity: turnHandle.opacity
        visible: opacity>0
    }


    //STBY circle
    Rectangle {
        z: -1000
        property bool active: selected||dragging||hover
        anchors.centerIn: parent
        width: mapProvider.metersToX(gy)*map.constSceneXY*2*turnR
        height: width
        color: active?"#2040FF40":"#100000FF"
        border.color: "#500000FF"
        border.width: 1*mapProvider.mapScaleFactor
        radius: width*0.5
        visible: width>parent.height*2
        //opacity: selected?1:0.5
        Behavior on color { ColorAnimation {duration: map.animation_duration*2; } }
    }
}
