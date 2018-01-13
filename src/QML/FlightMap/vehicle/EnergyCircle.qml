import QtQuick 2.5
import QtLocation 5.6
import QtPositioning 5.6

MapCircle {
    color: "transparent"
    border.color: rising?cRising:cLoosing
    border.width: 2
    smooth: true
    //opacity: 0.9
    property color cRising: "#A000FFFF"
    property color cLoosing: "#A0FF0000"

    //Fact bindings
    property real lat: m.gps_lat.value
    property real lon: m.gps_lon.value
    property real altitude: m.altitude.value
    property real ldratio: m.ldratio.value
    property real windHdg: m.windHdg.value
    property real windSpd: m.windSpd.value
    property real airspeed: m.airspeed.value
    property real cas2tas: m.cas2tas.value
    property real venergy: m.venergy.value

    //calculate Energy Circle

    property int range: altitude*ldratio
    property int distance: airspeed>0?(range*windSpd/(airspeed*(cas2tas>0?cas2tas:1))):0

    onDistanceChanged: updateTimer.running=true
    onRangeChanged: updateTimer.running=true

    Timer {
        id: updateTimer
        interval: range<2000?100:1000
        running: false
        repeat: false
        onTriggered: {
            center=QtPositioning.coordinate(lat,lon).atDistanceAndAzimuth(distance,windHdg)
            radius=range
        }
    }

    //center: QtPositioning.coordinate(lat,lon).atDistanceAndAzimuth(distance,windHdg)
    //visual
    visible: airspeed>1

    property bool rising: true
    Component.onCompleted: {
        rising=Qt.binding(function(){return venergy>0.5?true:venergy<-0.5?false:rising})
    }
}
