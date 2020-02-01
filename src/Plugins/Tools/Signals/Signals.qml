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
                values: [ mandala.cmd.att.roll, mandala.est.att.roll ]
            }
            SignalButton {
                text: "P"
                values: [ mandala.cmd.att.pitch, mandala.est.att.pitch ]
            }
            SignalButton {
                text: "Y"
                values: [ mandala.cmd.pos.course, mandala.cmd.att.yaw, mandala.est.att.yaw ]
            }
            SignalButton {
                text: "Axy"
                values: [ mandala.est.rel.ax, mandala.est.rel.ay ]
            }
            SignalButton {
                text: "Az"
                values: [ mandala.est.rel.az ]
            }
            SignalButton {
                text: "G"
                values: [ mandala.est.att.p, mandala.est.att.q, mandala.est.att.r ]
            }
            SignalButton {
                text: "M"
                values: [ mandala.sns.mag.x, mandala.sns.mag.y, mandala.sns.mag.z, mandala.est.aux.mag ]
            }
            SignalButton {
                text: "Pt"
                values: [ mandala.est.air.altitude, mandala.est.air.vspeed, mandala.est.air.airspeed ]
            }
            SignalButton {
                text: "Ctr"
                values: [ mandala.ctr.stab.ail, mandala.ctr.stab.elv, mandala.ctr.eng.thr, mandala.ctr.stab.rud, mandala.ctr.stab.col, mandala.cmd.rc.roll, mandala.cmd.rc.pitch, mandala.cmd.rc.thr, mandala.cmd.rc.yaw ]
            }
            SignalButton {
                text: "Usr"
                values: [ mandala.est.usr.u1, mandala.est.usr.u2, mandala.est.usr.u3, mandala.est.usr.u4, mandala.est.usr.u5, mandala.est.usr.u6 ]
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
