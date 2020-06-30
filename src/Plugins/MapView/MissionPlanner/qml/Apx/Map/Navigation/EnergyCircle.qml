import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

MapCircle {
    id: circle
    color: "transparent"
    border.color: c
    border.width: 2

    property color c: rising?cRising:cLoosing
    Behavior on c { ColorAnimation { duration: 3000 } }

    property color cRising: "#A000FFFF"
    property color cLoosing: "#A0FF0000"

    //Fact bindings
    property real lat: mandala.est.pos.lat.value
    property real lon: mandala.est.pos.lon.value
    property real altitude: mandala.est.pos.altitude.value
    property real ldratio: mandala.est.air.ld.value
    property real windHdg: mandala.est.wind.heading.value
    property real windSpd: mandala.est.wind.speed.value
    property real airspeed: mandala.est.air.airspeed.value
    property real cas2tas: mandala.est.air.ktas.value
    property real venergy: mandala.est.air.vse.value

    //calculate Energy Circle
    property int range: altitude*ldratio
    property int distance: airspeed>0?(range*windSpd/(airspeed*(cas2tas>0?cas2tas:1))):0
    property var pos: QtPositioning.coordinate(lat,lon).atDistanceAndAzimuth(distance,windHdg)

    onPosChanged: updateTimer.start()
    onRangeChanged: updateTimer.start()

    Timer {
        id: updateTimer
        interval: range<2000?100:1000
        running: false
        repeat: false
        onTriggered: {
            circle.center=circle.pos
            circle.radius=circle.range
        }
    }

    //visual
    visible: airspeed>1

    property bool rising: true
    Component.onCompleted: {
        rising=Qt.binding(function(){return venergy>0.5?true:venergy<-0.5?false:rising})
    }
}
