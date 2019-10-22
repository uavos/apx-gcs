import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Map.Common 1.0

MapItemGroup {
    visible: valid

    //Fact bindings
    property real lat: m.gps_lat.value
    property real lon: m.gps_lon.value
    property real hmsl: m.gps_hmsl.value
    property real home_hmsl: m.home_hmsl.value

    property real yaw: m.yaw.value
    property real power_payload: m.power_payload.value

    property real cam_roll: m.cam_roll.value
    property real cam_pitch: m.cam_pitch.value
    property real cam_yaw: m.cam_yaw.value

    //calculate
    property bool valid: power_payload>0
                         && camAlt>0
                         && (cam_roll!==0 || cam_pitch!==0 || cam_yaw!==0)

    property bool bCamFront: cam_roll !== 0

    property real camAlt: hmsl-home_hmsl

    property real azimuth: bCamFront?yaw:cam_yaw

    property real distance: camAlt*Math.tan((90+cam_pitch)*Math.PI/180)

    property var uavCoordinate: QtPositioning.coordinate(lat,lon,hmsl)
    property var targetCoordinate: QtPositioning.coordinate(lat,lon,home_hmsl).atDistanceAndAzimuth(distance,azimuth)

    MapCircle {
        id: circle
        color: "#fff"
        border.width: 0
        radius: 200
        opacity: 0.3
        center: targetCoordinate

        Behavior on center {
            enabled: ui.smooth
            CoordinateAnimation {
                duration: 1000
            }
        }
    }

    MapIcon {
        name: "circle-medium"
        coordinate: circle.center
        color: circle.color
    }

    MapQuickItem {
        coordinate: circle.center

        anchorPoint.x: text.implicitWidth/2
        anchorPoint.y: -text.implicitHeight

        sourceItem: Text {
            id: text
            font.family: font_condenced
            //font.pixelSize: control.size
            text: apx.distanceToString(targetCoordinate.distanceTo(uavCoordinate))
            color: circle.color
        }
    }

}
