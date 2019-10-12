import QtQuick 2.12
import QtQuick.Layouts 1.12

import Apx.Application 1.0
import Apx.Map.MapView 1.0

import Apx.Common 1.0
import Apx.Controls 1.0

MapView {
    id: control

    readonly property real margins: 10

    //initial animation
    PropertyAnimation {
        running: true
        target: map
        property: "zoomLevel"
        from: 12
        to: 16.33
        duration: 1000
        easing.type: Easing.OutInCirc
    }

    Component.onCompleted: {
        application.registerUiComponent(map,"map")
        ui.main.add(control, GroundControl.Layout.Background)
        ui.main.add(main, GroundControl.Layout.Main)
    }
    onMapBackgroundItemLoaded: {
        application.registerUiComponent(item,"mapbase")
    }


    //Controls
    /*Item {
        z: 9999
        BoundingRect { item: toolBar }
        BoundingRect { item: missionList }
        BoundingRect { item: bottom }
        BoundingRect { item: info }
        BoundingRect { item: status }
    }*/


    Item {
        id: main
        RowLayout {
            id: toolBar
            z: 100
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: margins
            MissionTools {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            }
            MapTools { }
        }

        RowLayout {
            id: missionList
            z: 50
            anchors.left: parent.left
            anchors.top: toolBar.bottom
            anchors.bottom: info.top
            anchors.topMargin: margins
            anchors.bottomMargin: margins
            MissionListView {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                Layout.fillHeight: true
            }
        }

        RowLayout {
            id: info
            anchors.left: parent.left
            anchors.bottom: bottom.bottom
            anchors.bottomMargin: status.implicitHeight+margins
            Loader {
                id: wind
                active: m.windSpd.value>0
                asynchronous: true
                sourceComponent: Component { Wind { } }
                visible: wind.status===Loader.Ready
            }
        }
        RowLayout {
            id: bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            RowLayout {
                id: status
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft|Qt.AlignBottom
                MapInfo { }
            }
            NumbersBar {
                Layout.fillWidth: true
                settingsName: "map"
                defaults: [
                    {"bind": "altitude", "title": "ALT", "prec": "0"},
                ]
            }
        }
    }
}
