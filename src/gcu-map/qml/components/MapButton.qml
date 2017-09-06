import QtQuick 2.2
import QtGraphicalEffects 1.0
import "."

Item {
    id: buttonItem
    property int size: parent.width
    property string icons: "ionicons"
    property string icon
    property string overIcon
    property color color: "#FFFFFF" //"#000000"
    property color colorHover: "#A0FFA0" //"#376479" // //"#FFFFFF"
    visible: opacity
    property bool shadow: true

    Behavior on opacity { PropertyAnimation {duration: map.animation_duration*2; } }

    signal clicked()

    width: size
    height: size

    Image {
        id: image
        anchors.fill: parent
        source: "/icons/"+icons+"/"+icon+".svg"
        smooth: true
    }
    MouseArea {
        id: mouseArea
        anchors.fill: image
        enabled: true
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            //console.log("click button");
            buttonItem.clicked();
        }
    }
    ColorOverlay {
        id: imageColor
        anchors.fill: image
        source: image
        color: mouseArea.containsMouse?buttonItem.colorHover:buttonItem.color
    }
    DropShadow {
        id: imageShadow
        visible: shadow
        anchors.fill: image
        horizontalOffset: 3
        verticalOffset: 4
        radius: 8
        samples: 16
        color: "#A0000000"
        source: imageColor//imageGlow
    }
    Image {
        id: imageOver
        visible: overIcon
        anchors.right: image.right
        anchors.top: image.top
        width: image.width/2
        height: width
        source: overIcon?"/icons/"+icons+"/"+overIcon+".svg":""
        smooth: true
    }
    ColorOverlay {
        id: imageColorOver
        visible: overIcon
        anchors.fill: imageOver
        source: imageOver
        color: imageColor.color
    }
    Glow {
        id: imageGlowOver
        visible: overIcon
        anchors.fill: imageOver
        radius: 4
        samples: 8
        color: "#000000"
        source: imageColorOver
    }
}
