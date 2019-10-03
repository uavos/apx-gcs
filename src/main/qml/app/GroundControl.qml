import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQml 2.12

import Qt.labs.settings 1.0

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0
import Apx.Menu 1.0

import "qrc:/app"

/*

    Main layout QML.
    Copy this file to `~/Documents/UAVOS/Plugins` and edit to override.

*/

Item {
    id: groundControl


    readonly property int instrumentsHeight: width*0.2
    readonly property color sepColor: "#244"

    property alias mainLayout: mainLayout
    property alias instrumentsLayout: instrumentsLayout

    property bool showInstruments: true
    property bool showSignals: true

    Settings {
        id: settings
        category: "Activity"
        property alias state: groundControl.state
    }

    states: [
        State {
            name: "flight"
            property string icon: "airplane"
            PropertyChanges {
                target: groundControl;
                showInstruments: true
                showSignals: false
            }
        },
        State {
            name: "mission"
            property string icon: "map"
            PropertyChanges {
                target: groundControl;
                showInstruments: false
                showSignals: false
            }
        },
        State {
            name: "tuning"
            extend: "flight"
            property string icon: "settings"
            PropertyChanges {
                target: groundControl;
                showSignals: true
            }
        }
    ]
    state: "flight"

    property string mainState: state

    Activities {
        id: activityControl
        z: 10000
        anchors.top: parent.top
        anchors.right: parent.right
    }

    GridLayout {
        anchors.fill: parent
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            InstrumentsLayout {
                id: instrumentsLayout
                visible: showInstruments
                state: groundControl.state
                Layout.rightMargin: activityControl.width+3
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: instrumentsHeight
            }
            Rectangle { visible: showInstruments; Layout.fillWidth: true; implicitHeight: visible?1:0; border.width: 0; color: sepColor; }

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
