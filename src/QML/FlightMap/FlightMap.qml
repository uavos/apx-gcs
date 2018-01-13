import QtQuick          2.3
import QtQuick.Controls 2.1
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Window   2.2

import QtQml 2.2

import GCS.FactSystem   1.0
import GCS.Vehicles     1.0

import "."
import "./map"
import "./vehicle"
import "./mission"

Map {
    id: map

    property double itemsScaleFactor: 1.5

    property variant mouseCoordinate: mouseCoordinateRaw.isValid?mouseCoordinateRaw:center
    property variant mouseClickCoordinate: center

    //property Vehicle currentVehicle: app.vehicles.current

    property int lastX: -1
    property int lastY: -1
    property int pressX: -1
    property int pressY: -1
    property int jitterThreshold: 30

    property variant mouseCoordinateRaw: toCoordinate(Qt.point(mouseArea.mouseX, mouseArea.mouseY))


    function metersToPixels(m)
    {
        var coord = map.toCoordinate(Qt.point(map.width,map.height/2))
        var dist = center.distanceTo(coord)
        return m*(map.width/2)/dist;
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

    onCenterChanged:{
        //if (map.followme && map.center != positionSource.position.coordinate) map.followme = false
    }

    onZoomLevelChanged:{
        //if(follow) center=itemToFollow.coordinate
    }

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
            enabled: app.settings.smooth.value && (!blocked)
            CoordinateAnimation {
                id: coordAnimation
                duration: 500;
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
            if(Math.abs(p1.x-p2.x)>map.width*4 || Math.abs(p1.y-p2.y)>map.height*4){
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
        centerSet.set(coord)
    }
    function flickToCoordinate(coord)
    {
        centerSet.flick(coord)
    }
    Connections {
        target: gesture
        onPanStarted:       followStop()
        onFlickStarted:     followStop()
        //onPanFinished:      followStop()
        //onFlickFinished:    followStop()
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
            /*if (Math.abs(map.pressX - mouse.x ) < map.jitterThreshold
                    && Math.abs(map.pressY - mouse.y ) < map.jitterThreshold) {
                showMainMenu(mouseCoordinate);
            }*/

            //map.clearScene(0);
            console.log(mapLoader.requestCount);
        }
    }


    //VEHICLES
    VehicleItem {
        z: map.z+50
        vehicle: app.vehicles.LOCAL
    }
    MapItemView {
        model: app.vehicles.list.model
        delegate: VehicleItem { z: map.z+50 }
    }
    VehiclesList {
        z: map.z+100
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: wind.left
        anchors.margins: 10
    }

    //MISSION
    Connections {
        target: app.vehicles
        onVehicleRegistered: {
            //console.log(vehicle)
            var c=waypointsC.createObject(map,{"vehicle": vehicle})
            addMapItemView(c)
            //vehicle.destroyed.connect(function(){removeMapItemView(c)})
            //c=waypointsPathC.createObject(map,{"vehicle": vehicle})
            //addMapItemView(c)

        }
    }
    Component {
        id: waypointsC
        MapItemView {
            property Vehicle vehicle
            model: vehicle.mission.waypoints.model
            delegate: WaypointItem { }
        }
    }
    Component {
        id: waypointsPathC
        MapItemView {
            property Vehicle vehicle
            model: vehicle.mission.waypoints.model
            delegate: WaypointPath { }
        }
    }

    //Current vehicle items
    EnergyCircle { }
    CmdPosCircle { }
    Home { }
    LoiterCircle { }



    //Information
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

}
