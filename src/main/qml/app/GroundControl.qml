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

    readonly property real pfdAspectRatio: 2
    readonly property int pfdMinHeight: 0 //280

    readonly property int instrumentsHeight: Math.max(width*0.4/pfdAspectRatio,pfdMinHeight)
    readonly property color sepColor: "#244"

    property alias containerMain:   containerMain
    property alias containerTop:    containerTop
    property alias containerBottom: containerBottom
    property alias containerLeft:   containerLeft
    property alias containerRight:  containerRight

    property bool showInstruments: true
    property bool showPfd: true
    property bool showStatus: true
    property bool showNumbers: true
    property bool showNodes: true
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
                showNodes: false
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
                showNodes: true
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
        height: instrumentsHeight
    }

    function addMainItem(item)
    {
        item.parent=containerMain
        item.anchors.fill=containerMain
    }

    //CONTENT
    GridLayout {
        anchors.fill: parent
        //spacing: 0
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                id: instrumentsPanel
                visible: showInstruments
                spacing: 0
                Layout.rightMargin: activityControl.width+3
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: instrumentsHeight

                Loader {
                    active: visible
                    visible: showStatus
                    asynchronous: true
                    sourceComponent: Component { Status { } }
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.leftMargin: 3
                    Layout.rightMargin: 3
                }
                Rectangle { Layout.fillHeight: true; implicitWidth: showStatus?1:0; border.width: 0; color: sepColor; }

                Loader {
                    active: visible
                    visible: showPfd
                    asynchronous: true
                    sourceComponent: Component { Pfd { } }
                    Layout.fillHeight: true
                    Layout.preferredWidth: instrumentsHeight*pfdAspectRatio
                }
                Rectangle { Layout.fillHeight: true; implicitWidth: 1; border.width: 0; color: sepColor; }

                Loader {
                    active: visible
                    visible: showNumbers
                    asynchronous: true
                    sourceComponent: Component { NumbersBox { settingsName: "instruments" } }
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.leftMargin: 3
                    Layout.rightMargin: 3
                }
                Rectangle { Layout.fillHeight: true; implicitWidth: showNumbers?1:0; border.width: 0; color: sepColor; }

                Loader {
                    active: visible
                    visible: apx.tools.terminal?true:false
                    asynchronous: true
                    source: apx.tools.terminal.qmlPage
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                }
                Rectangle { Layout.fillHeight: true; implicitWidth: 1; border.width: 0; color: sepColor; }

                Loader {
                    id: commands
                    asynchronous: true
                    sourceComponent: Component { Commands { } }
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
                Rectangle { Layout.fillHeight: true; implicitWidth: 1; border.width: 0; color: sepColor; }

            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; border.width: 0; color: sepColor; }

            //MAIN PART
            Item {
                id: containerMain
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 200*ui.scale
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
