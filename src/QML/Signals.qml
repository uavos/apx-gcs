import QtQuick 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.settings 1.0

import "./signals"
import "./components"


Item {
    id: window


    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        SignalsView {
            id: signals
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
        }
        ButtonGroup {
            id: buttonGroup
            //buttons: bottomArea.buttons
        }

        Flow {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: 3
            spacing: 3
            CleanButton {
                text: "R"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.cmd_roll, m.roll ]})
            }
            CleanButton {
                text: "P"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.cmd_pitch, m.pitch ]})
            }
            CleanButton {
                text: "Y"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.cmd_course, m.cmd_yaw, m.yaw ]})
            }
            CleanButton {
                text: "Axy"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.Ax, m.Ay ]})
            }
            CleanButton {
                text: "Az"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.Az ]})
            }
            CleanButton {
                text: "G"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.p, m.q, m.r ]})
            }
            CleanButton {
                text: "M"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.Hx, m.Hy, m.Hz ]})
            }
            CleanButton {
                text: "Pt"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.altitude, m.vspeed, m.airspeed ]})
            }
            CleanButton {
                text: "Ctr"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.ctr_ailerons, m.ctr_elevator, m.ctr_throttle, m.ctr_rudder, m.ctr_collective, m.rc_roll, m.rc_pitch, m.rc_throttle, m.rc_yaw ]})
            }
            CleanButton {
                text: "Usr"
                checkable: true
                ButtonGroup.group: buttonGroup
                onActivated: signals.facts=Qt.binding(function(){return [ m.user1, m.user2, m.user3, m.user4, m.user5, m.user6 ]})
            }

            CleanButton {
                text: signals.speedFactorValue+"x"
                onClicked: signals.changeSpeed()
            }
        }
    }

    property string currentPage: buttonGroup.checkedButton.text

    Settings {
        category: "signals"
        property alias page: window.currentPage
    }
    Component.onCompleted: {
        for(var i=0;i<buttonGroup.buttons.length;++i){
            var b=buttonGroup.buttons[i]
            if(b.text!==window.currentPage)continue
            buttonGroup.checkedButton=b
            break
        }
        if(buttonGroup.checkedButton==null){
            buttonGroup.checkedButton=buttonGroup.buttons[0] //showPage("R")
        }
    }

}
