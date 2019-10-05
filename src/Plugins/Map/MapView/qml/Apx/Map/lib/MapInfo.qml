import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

RowLayout {
    Text {
        Layout.preferredWidth: height*8
        font.family: font_narrow
        color: "#fff"
        property var c: map.mouseCoordinate
        text: apx.latToString(c.latitude)+" "+apx.lonToString(c.longitude)
    }

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

}
