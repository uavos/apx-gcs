/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtLocation
import QtPositioning

Map {
    id: control


    signal moved()
    signal clicked(var coordinate)
    signal menuRequested()

    readonly property var mouseCoordinate: mouseCoordinateRaw.isValid?mouseCoordinateRaw:center
    property var mouseClickCoordinate: center
    property point mouseClickPoint: Qt.point(pressX, pressY)

    property var defaultCoordinate: QtPositioning.coordinate(37.406015,-122.045175)

    function showRegion(r)
    {
        followStop()
        resetFlicking()
        tilt=0
        bearing=0
        r.width*=1.5
        r.height*=1.5
        visibleRegion=r
    }

    function centerOn(coord) {
        resetFlicking();
        if(coord.latitude===0 || coord.longitude===0)return
        anim.stop()
        center=coord
    }


    // Behavior on zoomLevel { enabled: ui.smooth; NumberAnimation {duration: 500; easing.type: Easing.InOutQuad; } }

    onMoved: {
        anim.stop()
        followStop()
    }

    //smooth panning
    CoordinateAnimation {
        id: anim
        duration: ui.smooth?500:0
        direction: CoordinateAnimation.Shortest
        easing.type: Easing.InOutCubic
        target: control
        property: "center"
    }
    function showCoordinate(coord)
    {
        if(follow) return
        resetFlicking();
        if(coord.latitude===0 || coord.longitude===0)return
        if(isFarMove(center,coord)){
            anim.stop()
            centerOn(coord)
            return
        }
        anim.easing.type=Easing.InOutCubic
        if(!anim.running)anim.from=center
        anim.to=coord
        anim.start()
    }
    function flickToCoordinate(coord)
    {
        if(follow) return
        resetFlicking();
        anim.easing.type=Easing.Linear
        if(!anim.running)anim.from=center
        anim.to=center.atDistanceAndAzimuth(center.distanceTo(coord)*0.2,center.azimuthTo(coord))
        anim.start()
    }
    function isFarMove(from,to,factor)
    {
        var p1=fromCoordinate(from,false);
        var p2=fromCoordinate(to,false);
        if(typeof(factor)=='undefined')factor=8
        if(Math.abs(p1.x-p2.x)>width*factor || Math.abs(p1.y-p2.y)>height*factor){
            return true
        }
        return false
    }
    function resetFlicking()
    {
        drag.enabled=false
        drag.enabled=true
    }

    //---------------------------
    //item follow
    function followItem(item)
    {
        if(!item)return;
        itemToFollow=item;
        follow=true;
        centerOn(item.coordinate)
    }
    function followStop()
    {
        follow=false;
    }

    //follow map circle
    Rectangle {
        z: 1000
        border.color: "#222"
        border.width: 2
        color: "transparent"
        width: 13
        height: width
        anchors.centerIn: parent
        visible: follow
        radius: height/2
    }

    //Item follow
    property bool follow: false
    property MapQuickItem itemToFollow
    Connections {
        enabled: follow
        target: itemToFollow
        function onCoordinateChanged(){ if(follow) centerOn(itemToFollow.coordinate) }
    }

    //internal
    color: '#333'

    center: defaultCoordinate

    focus: true
    onCopyrightLinkActivated: Qt.openUrlExternally(link)

    // user controls
    /*PinchHandler {
        id: pinch
        target: null
        onActiveChanged: if (active) {
            control.startCentroid = control.toCoordinate(pinch.centroid.position, false)
        }
        onScaleChanged: (delta) => {
            control.zoomLevel += Math.log2(delta)
            control.alignCoordinateToPoint(control.startCentroid, pinch.centroid.position)
        }
        onRotationChanged: (delta) => {
            control.bearing -= delta
            control.alignCoordinateToPoint(control.startCentroid, pinch.centroid.position)
        }
        grabPermissions: PointerHandler.TakeOverForbidden
    }*/
    WheelHandler {
        id: wheel
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        // grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType | PointerHandler.ApprovesTakeOverByItems
        onWheel: (event) => {
            const loc = control.toCoordinate(wheel.point.position)
            switch (event.modifiers) {
                case Qt.NoModifier:
                    control.zoomLevel += event.angleDelta.y / 600
                    break
                case Qt.ShiftModifier:
                    control.bearing += event.angleDelta.y / 15
                    break
                case Qt.ControlModifier:
                    control.tilt += event.angleDelta.y / 15
                    break
            }
            control.alignCoordinateToPoint(loc, wheel.point.position)
        }
    }

    DragHandler {
        id: drag
        signal flickStarted
        signal flickEnded
        target: null
        onTranslationChanged: (delta) => control.pan(-delta.x, -delta.y)
        onActiveChanged: if (active) {
            control.followStop()
            flickAnimation.stop()
        } else {
            flickAnimation.restart(centroid.velocity)
        }
    }

    property vector3d animDest
    onAnimDestChanged: if (flickAnimation.running) {
        const delta = Qt.vector2d(animDest.x - flickAnimation.animDestLast.x, animDest.y - flickAnimation.animDestLast.y)
        control.pan(-delta.x, -delta.y)
        flickAnimation.animDestLast = animDest
    }

    Vector3dAnimation on animDest {
        id: flickAnimation
        property vector3d animDestLast
        from: Qt.vector3d(0, 0, 0)
        duration: 500
        easing.type: Easing.OutQuad
        onStarted: drag.flickStarted()
        onStopped: drag.flickEnded()

        function restart(vel) {
            stop()
            control.animDest = Qt.vector3d(0, 0, 0)
            animDestLast = Qt.vector3d(0, 0, 0)
            to = Qt.vector3d(vel.x / duration * 100, vel.y / duration * 100, 0)
            start()
        }
    }

    tilt: tiltHandler.persistentTranslation.y / -5
    DragHandler {
        id: tiltHandler
        minimumPointCount: 2
        maximumPointCount: 2
        target: null
        xAxis.enabled: false
        grabPermissions: PointerHandler.TakeOverForbidden
        onActiveChanged: if (active) flickAnimation.stop()
    }


    Shortcut {
        enabled: control.zoomLevel < control.maximumZoomLevel
        sequence: StandardKey.ZoomIn
        onActivated: control.zoomLevel = Math.round(control.zoomLevel + 1)
    }
    Shortcut {
        enabled: control.zoomLevel > control.minimumZoomLevel
        sequence: StandardKey.ZoomOut
        onActivated: control.zoomLevel = Math.round(control.zoomLevel - 1)
    }



    // Mouse
    property int lastX: -1
    property int lastY: -1
    property int pressX: -1
    property int pressY: -1
    property int jitterThreshold: 30
    property var mouseCoordinateRaw: toCoordinate(Qt.point(mouseArea.mouseX, mouseArea.mouseY))

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        propagateComposedEvents: false

        onPressed : (mouse) => {
            forceActiveFocus()
            lastX = mouse.x
            lastY = mouse.y
            pressX = mouse.x
            pressY = mouse.y
            mouseClickCoordinate = toCoordinate(Qt.point(mouse.x, mouse.y))
        }

        onPositionChanged: (mouse) => {
            if (mouse.button == Qt.LeftButton) {
                lastX = mouse.x
                lastY = mouse.y
            }
        }

        onClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                control.clicked(mouseClickCoordinate)
            }else if (mouse.button === Qt.RightButton) {
                control.menuRequested()
            }
        }

        onDoubleClicked: (mouse) => {
            var mouseGeoPos = toCoordinate(Qt.point(mouse.x, mouse.y));
            var preZoomPoint = fromCoordinate(mouseGeoPos, false);
            if (mouse.button === Qt.LeftButton) {
                zoomLevel = Math.floor(zoomLevel + 1)
            } else if (mouse.button === Qt.RightButton) {
                zoomLevel = Math.floor(zoomLevel - 1)
            }
            var postZoomPoint = fromCoordinate(mouseGeoPos, false);
            var dx = postZoomPoint.x - preZoomPoint.x;
            var dy = postZoomPoint.y - preZoomPoint.y;

            center = toCoordinate(Qt.point(width / 2.0 + dx, height / 2.0 + dy));

            lastX = -1;
            lastY = -1;
        }

        onPressAndHold: (mouse) => {
            if (Math.abs(pressX - mouse.x ) < jitterThreshold
                    && Math.abs(pressY - mouse.y ) < jitterThreshold) {
                control.menuRequested()
            }
        }
    }
}
