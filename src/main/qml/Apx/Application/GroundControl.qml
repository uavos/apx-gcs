import QtQuick 2.12
import QtQuick.Layouts 1.12

import Qt.labs.settings 1.0

import "qrc:/Apx/Application"

/*

    Main layout QML.
    Copy this file to `~/Documents/UAVOS/Plugins` and edit to override.

*/


Item {
    id: groundControl


    enum Layout {
        Instrument,
        Background,
        ToolBar,
        Tool,
        Info,
        Status
    }
    function add(item, layout, index)
    {
        if(instrumentsLayout.add(item, layout, index))
            return
        if(mainLayout.add(item, layout, index))
            return
        console.error("Unsupported item position:", layout, item)
    }

    //internal
    readonly property int instrumentsHeight: width*0.2
    readonly property color sepColor: "#244"

    Settings {
        id: settings
        category: "Layout"
        property alias state: groundControl.state
    }

    state: "normal"
    states: [
        State {
            name: "normal"
            PropertyChanges {
                target: instrumentsLayout;
                visible: true
            }
        },
        State {
            name: "maximized"
            PropertyChanges {
                target: instrumentsLayout;
                visible: false
            }
        }
    ]

    property bool maximized: state=="maximized"
    function toggleState()
    {
        state=maximized?"normal":"maximized"
    }

    GridLayout {
        anchors.fill: parent
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            InstrumentsLayout {
                id: instrumentsLayout
                state: groundControl.state
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: instrumentsHeight
            }
            Rectangle { visible: instrumentsLayout.visible; Layout.fillWidth: true; implicitHeight: visible?1:0; border.width: 0; color: sepColor; }

            MainLayout {
                id: mainLayout
                state: groundControl.state
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 200*ui.scale
            }
        }
    }
}
