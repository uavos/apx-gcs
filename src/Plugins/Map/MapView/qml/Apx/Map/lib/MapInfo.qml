import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

RowLayout {

    // mouse coordinate
    Text {
        Layout.preferredWidth: height*8
        font.family: font_narrow
        color: "#fff"
        property var c: map.mouseCoordinate
        text: apx.latToString(c.latitude)+" "+apx.lonToString(c.longitude)
    }

    // map scale and distance measure
    Item {
        implicitWidth: loader.implicitWidth
        implicitHeight: loader.implicitHeight
        Loader {
            id: loader
            asynchronous: true
            property int idx: 0
            sourceComponent: scale
            Component {
                id: scale
                MapScale { width: 100 }
            }

            Component {
                id: dist
                MapDistance { width: 100 }
            }
            function advance()
            {
                active=false
                switch(++idx){
                default:
                    sourceComponent=scale
                    idx=0
                    break
                case 1:
                    sourceComponent=dist
                    break
                }
                active=true
            }
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: loader.advance()
        }
    }

    // travel path
    Item {
        implicitHeight: 20
        implicitWidth: Math.max(icon.width+text.implicitWidth, height*4)

        MaterialIcon {
            id: icon
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            name: "airplane"
            rotation: 90
            size: height
        }
        Text {
            id: text
            anchors.left: icon.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            font.family: font_condenced
            color: "#fff"
            text: apx.distanceToString(apx.vehicles.current.totalDistance)
        }
        ToolTipArea {
            text: qsTr("Distance travelled")
            cursorShape: Qt.PointingHandCursor
            onClicked: apx.vehicles.current.telemetry.rpath.trigger()
        }
    }


}
