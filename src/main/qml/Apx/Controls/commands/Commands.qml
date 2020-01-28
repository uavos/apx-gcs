import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0
import Apx.Menu 1.0


Rectangle {
    id: root

    readonly property var f_mode: mandala.cmd.op.mode


    border.width: 0
    color: "#000"
    implicitWidth: 400
    implicitHeight: 400

    onWidthChanged: implicitHeight=width
    onHeightChanged: implicitWidth=height

    readonly property int margins: 3

    property var vehicle: apx.vehicles.current

    //sizes
    readonly property int buttonHeight: width*0.1//,32)

    property int size: Math.max(50,width)*0.08

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.margins
        spacing: buttonHeight/4


        RowLayout {
            Layout.alignment: Qt.AlignTop|Qt.AlignLeft
            height: buttonHeight

            FactValue {
                defaultHeight: buttonHeight
                ui_scale: 1
                fact: vehicle
                iconName: fact.icon
                toolTip: apx.vehicles.title
                showTitle: false
                valueScale: 0.6
                value: vehicle.title
                active: false
                warning: vehicle.streamType<=0
                enabled: true
            }

            FactValue {
                defaultHeight: buttonHeight
                ui_scale: 1
                fact: vehicle.nodes
                showTitle: false
                showIcon: true
                valueScale: 0.6
                value: fact.nodesCount
                iconName: fact.icon
                warning: fact.nodesCount<=0
                active: fact.modified || fact.progress>=0 || (!fact.dataValid)
                enabled: true
            }

            FactValue {
                defaultHeight: buttonHeight
                ui_scale: 1
                fact: vehicle.mission
                showTitle: false
                showIcon: true
                valueScale: 0.6
                value: fact.missionSize
                iconName: fact.icon
                warning: fact.missionSize<=0
                active: fact.modified || fact.progress>=0
                enabled: true
            }
        }

        //Mode change row
        StackView {
            clip: true
            Layout.fillWidth: true
            Layout.preferredHeight: buttonHeight
            property string mode: f_mode.text
            onModeChanged: {
                var modes
                var body
                switch(f_mode.value){
                default: modes=f_mode.enumStrings; break;
                case op_mode_EMG:
                    modes=["RPV","TAXI"]
                    body="EMG"
                    break
                case op_mode_RPV:
                    modes=["UAV","WPT"]
                    break
                case op_mode_UAV:
                    modes=["WPT","LANDING"]
                    break
                case op_mode_WPT:
                    modes=["STBY","LANDING"]
                    break
                case op_mode_HOME:
                    modes=["WPT","LANDING"]
                    break
                case op_mode_STBY:
                    modes=["WPT","LANDING"]
                    break
                case op_mode_TAXI:
                    modes=["TAKEOFF","EMG"]
                    break
                case op_mode_TAKEOFF:
                    modes=["WPT","STBY"]
                    break
                case op_mode_LANDING:
                    modes=["WPT","STBY"]
                    break
                }
                replace(null,"Mode.qml",{"modes": modes, "body": body },StackView.PushTransition)
            }
        }

        //body controls area
        Pages {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        //menus
        Ctr {
            Layout.fillWidth: true
            Layout.preferredHeight: buttonHeight
        }

    }
}
