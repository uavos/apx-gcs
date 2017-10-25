import QtQuick 2.2
import QtGraphicalEffects 1.0
import "."
import "../"
import "../components"
import com.uavos.map 1.0


MapObject {
    id: runwayItem
    mItem: model.modelData
    parent: missionItem

    property var f_turn: mItem.child("turn")
    property var f_HMSL: mItem.child("HMSL")
    property var f_approach: mItem.child("approach")

    property int turn: f_turn?m.f_turn.value:0
    property int hmsl: f_HMSL?m.f_HMSL.value:0
    property int approach: f_approach?m.f_approach.value:0


    property bool current: m.rwidx.value === index
    property bool landing: m.mode.value === mode_LANDING && current


    text: (approach>0?"":"H")+mItem.name
    color: Style.cBlue
    textColor: "white"

    //right area info
    Column{
        id: infoItem
        z: -10
        spacing: 1
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: (parent.width+width/2)/map.tmpScale
        scale: 1/map.tmpScale
        visible: mapProvider.level>13
        MapText {
            textColor: "white"
            color: current?Style.cBlue:Style.cNormal
            text: (turn===0?"L":"R")+mItem.heading.toFixed()
            opacity: (!dragging)?((hover||selected)?1:0.6):0
            scale: 1
        }
        MapText {
            visible: hmsl!=0
            textColor: "white"
            color: Style.cGreen
            text: hmsl+"m"
            opacity: (!dragging)?((hover||selected)?1:0.6):0
            scale: 1
        }
    }

    //runway end point
    MapObject {
        id: endPointItem
        parent: missionItem
        parentObject: runwayItem
        lat: runwayItem.mItem.rwEndPoint.x
        lon: runwayItem.mItem.rwEndPoint.y
        onObjectMoved: {
            snapFromModel(mlat,mlon);
            runwayItem.mItem.rwEndPoint=Qt.point(mlat,mlon);
        }
        color: "white"
        textColor: "black"
        opacity: 0.8
        //top area info - angle
        MapText {
            visible: endPointItem.dragging
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -(parent.height+height)/map.tmpScale
            textColor: "white"
            color: current?Style.cBlue:Style.cNormal
            text: (turn===0?"L":"R")+mItem.heading.toFixed()
            opacity: (!dragging)?((hover||selected)?1:0.6):0
        }
    }
    //runway app point
    MapObject {
        id: rwApp
        z: -99
        visible: mapProvider.level>10
        parent: missionItem
        parentObject: runwayItem
        lat: runwayItem.mItem.rwAppPoint.x
        lon: runwayItem.mItem.rwAppPoint.y
        color: "white"
        textColor: "black"
        opacity: v>0?0.8:0.5
        property int v: approach.toFixed()
        text: v>0?(v+"m"):"H----"
        property double r: runwayItem.mItem.heading-90;
        rotation: v>0?mandala.angle90(r):mandala.angle(r);
        onObjectMoved: {
            snapFromModel(mlat,mlon);
            runwayItem.mItem.rwAppPoint=Qt.point(mlat,mlon);
        }
    }
    //approach delta rectangle
    Rectangle {
        id: deltaRect
        z: -999
        visible: landing && height>10
        property int v: m.delta.value
        Behavior on v { NumberAnimation {duration: map.animation_duration; } }
        anchors.left: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        property bool warn: v<-300
        property double sf: mapProvider.metersToX(gy)*map.constSceneXY
        width: Math.max(5,Math.abs(v*sf))
        height: 45*sf
        color: warn?"#60FF0000":(v>=150||v<-50)?"#40FFFF00":"#4000FF00"
        border.color: warn?"#E0FF0000":"#C000FF00"
        border.width: 1
        radius: Math.min(5,height*0.1)
        transform: Rotation {
            origin.x: 0
            origin.y: deltaRect.height/2
            axis.z: 1
            angle: runwayItem.mItem.heading+(deltaRect.v>=0?-90:180-90)
        }
        Behavior on color { ColorAnimation {duration: map.animation_duration*2; } }
    }

    //landing runway background arrows
    Column {
        id: arrowsPath
        z: -90
        visible: landing && sz>20
        anchors.bottom: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        property double sf: mapProvider.metersToX(gy)*map.constSceneXY
        property double sz: 40*sf
        property double step: 100*sf
        spacing: step-sz
        transform: Rotation {
            origin.x: arrowsPath.sz/2
            origin.y: arrowsPath.height
            axis.z: 1
            angle: runwayItem.mItem.heading
        }
        Repeater {
            model: Math.min(10,Math.max(1,Math.floor(Math.sqrt(Math.pow(Math.abs(runwayItem.px-endPointItem.px),2)+Math.pow(Math.abs(runwayItem.py-endPointItem.py),2))/arrowsPath.step)))
            FastBlur {
                width: arrowsPath.sz
                height: width
                radius: 8
                opacity: 0.8
                source: ColorOverlay {
                    width: arrowsPath.sz
                    height: width
                    source: Image {
                        width: arrowsPath.sz
                        height: width
                        source: "/icons/ionicons/ios-arrow-up.svg"
                    }
                    color: "white"
                }
            }
        }
    }


    QmlMapPath {
        z: -101
        parent: missionItem
        provider: mapProvider
        visible: current
        lineWidth: 12
        color: landing?"blue":Style.cBlue
        //opacity: 1
        path: mItem.pathRw
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
    }
    QmlMapPath {
        z: -100
        parent: missionItem
        provider: mapProvider
        lineWidth: 6
        color: "white"
        opacity: 0.8
        path: mItem.pathRw
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
    }
    QmlMapPath {
        z: -99
        parent: missionItem
        provider: mapProvider
        visible: mapProvider.level>13
        lineWidth: 3
        color: "black"
        opacity: 0.5
        path: mItem.pathRw
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
    }
    QmlMapPath {
        z: -100
        parent: missionItem
        provider: mapProvider
        visible: mapProvider.level>10
        lineWidth: 1
        color: "white"
        opacity: landing?1:0.6
        path: mItem.pathApp
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
        Behavior on opacity { PropertyAnimation {duration: map.animation_duration*2; } }
    }
    QmlMapPath {
        z: -100
        parent: missionItem
        provider: mapProvider
        visible: mapProvider.level>10
        lineWidth: landing?2:1
        color: "white"
        opacity: (landing||hover||rwApp.hover||dragging||rwApp.dragging)?0.8:0.6
        path: mItem.pathTA
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
        Behavior on opacity { PropertyAnimation {duration: map.animation_duration*2; } }
    }

}
