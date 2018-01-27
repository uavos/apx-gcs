import QtQuick          2.3
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Window   2.2

import QtQml 2.2
import QtGraphicalEffects 1.0

//PieMenu
import QtQuick.Extras 1.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4


import GCS.FactSystem   1.0
import GCS.Vehicles     1.0

import "../menu"
import "../pfd"

import "."
import "./map"
import "./vehicle"
import "./mission"
import "./menu"

Map {
    id: map

    property double itemsScaleFactor: 1

    property var mouseCoordinate: mouseCoordinateRaw.isValid?mouseCoordinateRaw:center
    property var mouseClickCoordinate: center

    property var selectedObject

    property int lastX: -1
    property int lastY: -1
    property int pressX: -1
    property int pressY: -1
    property int jitterThreshold: 30

    property var mouseCoordinateRaw: toCoordinate(Qt.point(mouseArea.mouseX, mouseArea.mouseY))


    signal clicked()
    signal mapMenuRequested()

    function metersToPixels(m)
    {
        var coord = map.toCoordinate(Qt.point(map.width,map.height/2))
        var dist = center.distanceTo(coord)
        return m*(map.width/2)/dist;
    }

    function showFactMenu(fact)
    {
        var c=factMenuC.createObject(map,{"fact": fact})
        c.open()
    }
    function showMapMenu()
    {
        mapMenuRequested();
        var c=mapMenuC.createObject(map,{"pos": Qt.point(pressX,pressY)})
        c.open()
    }

    Component {
        id: factMenuC
        FactMenuPopup {
            id: factMenu
            parent: map
            x: (parent.width/2 - width) / 2
            y: missionList.y
            //property var fact: app
            //menu: FactMenu { fact: factMenu.fact }
            onClosed: destroy()
        }
    }

    Component {
        id: mapMenuC
        FactMenuPopup {
            id: mapMenu
            parent: map
            property point pos
            x: pos.x-width/2 //(parent.width/2 - width) / 2
            y: pos.y-100 //missionList.y
            fact: app.vehicles.current.mission.tools.map
            closeOnTrigger: true
            showTitle: false
            onClosed: destroy()
        }
    }


    //Map componnet parameters
    color: '#333'
    plugin: Plugin { name: "uavos" }
    center: QtPositioning.coordinate(47.2589912414551,11.3327512741089)
    zoomLevel: 14
    activeMapType: supportedMapTypes[0]
    //fieldOfView: 15

    gesture.acceptedGestures: MapGestureArea.PanGesture | MapGestureArea.FlickGesture | MapGestureArea.PinchGesture | MapGestureArea.RotationGesture | MapGestureArea.TiltGesture
    gesture.flickDeceleration: 30
    gesture.enabled: true

    focus: true
    onCopyrightLinkActivated: Qt.openUrlExternally(link)

    //Item follow
    property bool follow: false
    property MapQuickItem itemToFollow
    Location {
        id: centerSet
        //coordinate: QtPositioning.coordinate(47.2589912414551,11.3327512741089)
        Behavior on coordinate {
            id: centerSetSmooth
            property bool blocked: false
            property alias easing: coordAnimation.easing
            //property alias duration: coordAnimation.duration
            property alias running: coordAnimation.running
            enabled: !blocked
            CoordinateAnimation {
                id: coordAnimation
                duration: app.settings.smooth.value?500:0;
                direction: CoordinateAnimation.Shortest;
                easing.type: Easing.InOutCubic
            }
        }
        onCoordinateChanged: {
            if(centerSetSmooth.enabled)
                map.center=centerSet.coordinate
        }
        function set(coord)
        {
            var p1=map.fromCoordinate(map.center,false);
            var p2=map.fromCoordinate(coord,false);
            if(Math.abs(p1.x-p2.x)>map.width*10 || Math.abs(p1.y-p2.y)>map.height*10){
                map.center=coord
                return
            }
            centerSetSmooth.blocked=true
            centerSetSmooth.easing.type=Easing.InOutCubic
            //centerSetSmooth.duration=500
            coordinate=map.center
            centerSetSmooth.blocked=false
            coordinate=coord
        }
        function flick(coord)
        {
            if(!centerSetSmooth.running){
                centerSetSmooth.blocked=true
                coordinate=map.center
                centerSetSmooth.blocked=false
            }
            centerSetSmooth.easing.type=Easing.Linear
            //centerSetSmooth.duration=1000
            coordinate=map.center.atDistanceAndAzimuth(map.center.distanceTo(coord)*0.2,map.center.azimuthTo(coord))
        }
    }
    function followItem(item)
    {
        if(!item)return;
        itemToFollow=item;
        follow=true;
        center=item.coordinate
    }
    function followStop()
    {
        follow=false;
    }
    function centerOnItem(item)
    {
        centerOnCoordinate(item.coordinate)
    }
    function centerOnCoordinate(coord)
    {
        if(follow)return;
        centerSet.set(coord)
    }
    function flickToCoordinate(coord)
    {
        if(follow)return;
        centerSet.flick(coord)
    }
    Connections {
        target: gesture
        onPanStarted:       followStop()
        onFlickStarted:     followStop()
    }
    Connections {
        enabled: follow
        target: itemToFollow
        onCoordinateChanged: if(follow) center=itemToFollow.coordinate
    }


    //Keyboard controls
    Keys.onPressed: {
        if (event.key === Qt.Key_Plus) {
            map.zoomLevel++;
        } else if (event.key === Qt.Key_Minus) {
            map.zoomLevel--;
        } else if (event.key === Qt.Key_Left || event.key === Qt.Key_Right ||
                   event.key === Qt.Key_Up   || event.key === Qt.Key_Down) {
            var dx = 0;
            var dy = 0;

            switch (event.key) {

            case Qt.Key_Left: dx = map.width / 4; break;
            case Qt.Key_Right: dx = -map.width / 4; break;
            case Qt.Key_Up: dy = map.height / 4; break;
            case Qt.Key_Down: dy = -map.height / 4; break;

            }

            var mapCenterPoint = Qt.point(map.width / 2.0 - dx, map.height / 2.0 - dy);
            map.center = map.toCoordinate(mapCenterPoint);
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        propagateComposedEvents: true

        onPressed : {
            map.lastX = mouse.x
            map.lastY = mouse.y
            map.pressX = mouse.x
            map.pressY = mouse.y
            mouseClickCoordinate = map.toCoordinate(Qt.point(mouse.x, mouse.y))
        }

        onPositionChanged: {
            if (mouse.button == Qt.LeftButton) {
                map.lastX = mouse.x
                map.lastY = mouse.y
            }
        }

        onClicked: {
            if (mouse.button === Qt.LeftButton) {
                map.clicked()
            }else if (mouse.button === Qt.RightButton) {
                showMapMenu()
            }
        }

        onDoubleClicked: {
            var mouseGeoPos = map.toCoordinate(Qt.point(mouse.x, mouse.y));
            var preZoomPoint = map.fromCoordinate(mouseGeoPos, false);
            if (mouse.button === Qt.LeftButton) {
                map.zoomLevel = Math.floor(map.zoomLevel + 1)
            } else if (mouse.button === Qt.RightButton) {
                map.zoomLevel = Math.floor(map.zoomLevel - 1)
            }
            var postZoomPoint = map.fromCoordinate(mouseGeoPos, false);
            var dx = postZoomPoint.x - preZoomPoint.x;
            var dy = postZoomPoint.y - preZoomPoint.y;

            var mapCenterPoint = Qt.point(map.width / 2.0 + dx, map.height / 2.0 + dy);
            map.center = map.toCoordinate(mapCenterPoint);

            lastX = -1;
            lastY = -1;
        }

        onPressAndHold:{
            if (Math.abs(map.pressX - mouse.x ) < map.jitterThreshold
                    && Math.abs(map.pressY - mouse.y ) < map.jitterThreshold) {
                showMapMenu();
            }
        }
    }


    //VEHICLES
    VehicleItem {
        z: map.z+80
        vehicle: app.vehicles.LOCAL
    }
    MapItemView {
        model: app.vehicles.list.model
        delegate: VehicleItem { z: map.z+81 }
    }

    //Current vehicle items
    EnergyCircle { }
    CmdPosCircle { }
    Home { }
    LoiterCircle { }

    //Controls
    VehiclesList {
        id: vehiclesList
        z: map.z+100
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: wind.left
        anchors.margins: 10
    }

    MissionList {
        id: missionList
        z: map.z+100
        anchors.top: vehiclesList.bottom
        anchors.left: parent.left
        anchors.bottom: parent.bottom //btm.top
        anchors.margins: 10

    }

    /*FastBlur {
        id: fastBlur
        anchors.top: vehiclesList.bottom
        anchors.left: parent.left
        anchors.bottom: parent.bottom //btm.top
        anchors.margins: 10
        width: missionList.width
        radius: 40
        opacity: 0.55

        source: ShaderEffectSource {
            sourceItem: map
            sourceRect: Qt.rect(0, 0, fastBlur.width, fastBlur.height)
        }
    }*/

    /*Rectangle {
        id: btm
        z: 1000
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        height: map.height*0.3
        width: height*1.6
        border.width: 1
        border.color: "#fff"
        radius: 8
        color: "transparent"
        clip: true
        Pfd {
            z: parent.z-1
            anchors.fill: parent
            anchors.margins: 1
            //asynchronous: app.settings.smooth.value
            //source: "qrc:///pfd/Pfd.qml"
        }
    }*/


    //Information Right area
    Column {
        z: map.z+100
        anchors.bottom: parent.bottom;
        anchors.right: parent.right
        anchors.margins: 20

        MapBusy {
            anchors.right: parent.right
        }
        MapScale {
            anchors.right: parent.right
        }
    }
    Wind {
        id: wind
        z: map.z+100
        anchors.top: map.top
        anchors.right: map.right
        anchors.margins: 10
    }

    /*MapStatusBar {
        z: map.z+100
        anchors.bottom: map.bottom
        anchors.left: map.left
        anchors.right: map.right
        height: 12*itemsScaleFactor
    }*/

    // The code below enables SSAA
    /*layer.enabled: true
    layer.smooth: true
    layer.samples: 4
    property int w : map.width
    property int h : map.height
    property int pr: Screen.devicePixelRatio
    layer.textureSize: Qt.size(w  * 2 * pr, h * 2 * pr)
    layer.effect: ShaderEffect {
        fragmentShader: "
            uniform lowp sampler2D source; // this item
            uniform lowp float qt_Opacity; // inherited opacity of this item
            varying highp vec2 qt_TexCoord0;
            void main() {
                lowp vec4 p = texture2D(source, qt_TexCoord0);
                lowp float g = dot(p.xyz, vec3(0.344, 0.5, 0.156));
                gl_FragColor = vec4(g, g, g, p.a) * qt_Opacity;
            }"
    }*/

}
