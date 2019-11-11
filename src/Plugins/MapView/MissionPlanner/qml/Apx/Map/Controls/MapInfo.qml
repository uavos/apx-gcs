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

    //tiles downloader
    BusyIndicator {
        id: busy
        property var fact: apx.tools?apx.tools.location:null
        property string text: fact?fact.status:""
        property int progress: fact?fact.progress:-1

        padding: 1
        implicitHeight: 20
        implicitWidth: implicitHeight
        running: progress>=0
        Text {
            anchors.fill: parent
            color: "#60FFFFFF"
            text: busy.text
            font.bold: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            font.pixelSize: Math.max(8,height*0.6)
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                busy.fact.abort();
            }
        }
    }

    FactValue {
        implicitHeight: 20
        fact: apx.tools?apx.tools.location.offline:null
        showTitle: false
        showIcon: false
        value: (active?qsTr("offline"):qsTr("online")).toUpperCase()
        active: fact.value
        enabled: true
        onTriggered: fact.value=!fact.value
        visible: active
        onActiveChanged: visible=true
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
        ToolTipArea {
            cursorShape: Qt.PointingHandCursor
            onClicked: loader.advance()
            text: qsTr("Distance measurement tool")
        }
    }

    // travel path
    Item {
        id: control
        implicitHeight: 20
        implicitWidth: Math.max(icon.width+textItem.implicitWidth, height*4)

        opacity: apx.vehicles.current.totalDistance>0?1:0.5
        property string text: apx.distanceToString(apx.vehicles.current.totalDistance)
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
            id: textItem
            anchors.left: icon.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            font.family: font_condenced
            color: "#fff"
            text: control.text
        }
        ToolTipArea {
            text: qsTr("Distance travelled")
            cursorShape: Qt.PointingHandCursor
            onClicked: apx.vehicles.current.telemetry.rpath.trigger()
        }
    }




}
