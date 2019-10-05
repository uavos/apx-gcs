import QtQuick 2.5
import QtLocation 5.6
import QtPositioning 5.6

MapCircle {
    color: "#fff"
    border.width: 0
    radius: 200
    opacity: 0.3

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

    center: QtPositioning.coordinate(lat,lon).atDistanceAndAzimuth(distance,azimuth)

    //visual
    visible: valid
    Behavior on center {
        enabled: ui.smooth
        CoordinateAnimation {
            duration: 1000
            easing.type: Easing.InOutCubic
        }
    }
}
