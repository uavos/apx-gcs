import QtQuick 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.settings 1.0

import Apx.Common 1.0


Rectangle {
    id: control

    implicitHeight: layout.implicitHeight
    implicitWidth: layout.implicitWidth

    border.width: 0
    color: "#000"
    //radius: 5

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0
        SignalsView {
            id: signals
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130*ui.scale
        }
        ButtonGroup {
            id: buttonGroup
            //buttons: bottomArea.buttons
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: 3
            spacing: 3
            Layout.maximumHeight: 22*ui.scale
            SignalButton {
                text: "R"
                values: [ m.cmd_roll, m.roll ]
            }
            SignalButton {
                text: "P"
                values: [ m.cmd_pitch, m.pitch ]
            }
            SignalButton {
                text: "Y"
                values: [ m.cmd_course, m.cmd_yaw, m.yaw ]
            }
            SignalButton {
                text: "Axy"
                values: [ m.Ax, m.Ay ]
            }
            SignalButton {
                text: "Az"
                values: [ m.Az ]
            }
            SignalButton {
                text: "G"
                values: [ m.p, m.q, m.r ]
            }
            SignalButton {
                text: "M"
                values: [ m.Hx, m.Hy, m.Hz ]
            }
            SignalButton {
                text: "Pt"
                values: [ m.altitude, m.vspeed, m.airspeed ]
            }
            SignalButton {
                text: "Ctr"
                values: [ m.ctr_ailerons, m.ctr_elevator, m.ctr_throttle, m.ctr_rudder, m.ctr_collective, m.rc_roll, m.rc_pitch, m.rc_throttle, m.rc_yaw ]
            }
            SignalButton {
                text: "Usr"
                values: [ m.user1, m.user2, m.user3, m.user4, m.user5, m.user6 ]
            }

            CleanButton {
                titleSize: 0.8
                text: signals.speedFactorValue+"x"
                onClicked: signals.changeSpeed()
                Layout.fillHeight: true
                Layout.minimumWidth: height*3
            }
        }
    }

    property string currentPage: buttonGroup.checkedButton.text

    Settings {
        category: "signals"
        property alias page: control.currentPage
    }
    Component.onCompleted: {
        for(var i=0;i<buttonGroup.buttons.length;++i){
            var b=buttonGroup.buttons[i]
            if(b.text!==control.currentPage)continue
            buttonGroup.checkedButton=b
            break
        }
        if(buttonGroup.checkedButton==null){
            buttonGroup.checkedButton=buttonGroup.buttons[0] //showPage("R")
        }
    }

}
