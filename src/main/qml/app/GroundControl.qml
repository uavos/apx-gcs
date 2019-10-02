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

    property alias containerTop:    containerTop
    property alias containerBottom: containerBottom
    property alias containerLeft:   containerLeft
    property alias containerRight:  containerRight

    property bool showInstruments: true
    property bool showSignals: true

    readonly property real containerMargins: 10*ui.scale

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

    function addMainPlugin(plugin)
    {
        plugin.parent=mainPanel
        plugin.anchors.fill=mainPanel
    }

    function addInstrumentPlugin(plugin)
    {
        instrumentsPanel.addPlugin(plugin)
    }


    //CONTENT
    GridLayout {
        anchors.fill: parent
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            InstrumentsPanel {
                id: instrumentsPanel
                visible: showInstruments
                Layout.rightMargin: activityControl.width+3
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: instrumentsHeight
            }
            Rectangle { visible: showInstruments; Layout.fillWidth: true; implicitHeight: visible?1:0; border.width: 0; color: sepColor; }

            //MAIN PART
            Item {
                id: mainPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 200*ui.scale
                BoundingRect { anchors.fill: containerTop }
                BoundingRect { anchors.fill: containerBottom }
                BoundingRect { anchors.fill: containerLeft }
                BoundingRect { anchors.fill: containerRight }
                RowLayout {
                    id: containerTop
                    z: 100
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: containerMargins
                    anchors.rightMargin: showInstruments?0:activityControl.width+containerMargins
                    VehiclesListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                    TaskBar {
                        Layout.fillWidth: false
                        Layout.fillHeight: false
                        Layout.preferredHeight: 32*ui.scale
                        Layout.alignment: Qt.AlignTop | Qt.AlignRight
                    }
                }
                ColumnLayout {
                    id: containerLeft
                    z: 100
                    anchors.left: parent.left
                    anchors.top: containerTop.bottom
                    anchors.bottom: parent.bottom
                    anchors.margins: containerMargins
                    Loader {
                        id: loaderMission
                        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                        Layout.fillHeight: true
                        sourceComponent: Component { MissionListView { } }
                    }
                    Loader {
                        id: loaderSignals
                        active: showSignals
                        Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                        sourceComponent: Component { Signals { } }
                        visible: active
                    }
                }
                RowLayout {
                    id: containerBottom
                    z: 100
                    anchors.left: containerLeft.right
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: containerMargins
                    Loader {
                        active: visible
                        asynchronous: true
                        sourceComponent: Component {
                            NumbersBar {
                                layoutDirection: Qt.RightToLeft
                                settingsName: "map"
                                defaults: [
                                    {"bind": "altitude", "title": "ALT", "prec": "0"},
                                ]
                            }
                        }
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                    }
                }
                ColumnLayout {
                    id: containerRight
                    z: 100
                    anchors.right: parent.right
                    anchors.top: containerTop.bottom
                    anchors.bottom: containerBottom.top
                    anchors.margins: containerMargins
                    anchors.rightMargin: (showInstruments?0:activityControl.width)+containerMargins
                }
            }
        }
    }
}
