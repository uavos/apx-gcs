import QtQuick 2.5;
import GCS.Vehicles 1.0
import QtGraphicalEffects 1.0
import "."

Item {
    id: windItem


    width: textItem.height*4
    property int arrowSize: width*0.5

    height: textItem.height+10+arrowSize


    //Fact bindings
    property real windSpd: m.windSpd.value
    property real windHdg: m.windHdg.value


    opacity: 0.8

    MapText {
        id: textItem
        color: "#fc5"
        textColor: "black"
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        //anchors.right: parent.right
        text: windSpd.toFixed(1)+" m/s"
    }
    MapSvgImage {
        id: image
        color: "#fff" //"#fd6"
        source: "./icons/wind-arrow.svg"
        sourceSize.height: arrowSize
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        rotation: v-map.bearing
        property real v: windHdg
        Behavior on v { enabled: app.settings.smooth.value; RotationAnimation {duration: 1000; direction: RotationAnimation.Shortest; } }
    }
}
