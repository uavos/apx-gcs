import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0
import Apx.Menu 1.0

ColumnLayout {
    id: root
    property var vehicle: apx.vehicles.current

    property int size: Math.max(50,height)*0.12

    spacing: 4
    //clip: true


    property int topRowHeight: Math.min(size*0.9,32)

    RowLayout {
        Layout.alignment: Qt.AlignTop|Qt.AlignLeft
        Layout.margins: 4
        height: topRowHeight

        FactValue {
            defaultHeight: topRowHeight
            ui_scale: 1
            fact: vehicle
            iconName: fact.icon
            toolTip: apx.vehicles.title
            showText: true
            text: vehicle.title
            active: false
            warning: vehicle.streamType<=0
            enabled: true
            onTriggered: vehicle.requestMenu()
        }

        FactValue {
            defaultHeight: topRowHeight
            ui_scale: 1
            fact: vehicle.nodes
            title: fact.nodesCount
            iconName: fact.icon
            warning: fact.nodesCount<=0
            active: fact.modified || fact.progress>=0 || (!fact.dataValid)
            enabled: true
            onTriggered: {
                if(fact.progress<0)fact.request()
                else fact.stop()
                fact.requestMenu()
            }
            onMenuRequested: fact.requestMenu()
        }

        FactValue {
            defaultHeight: topRowHeight
            ui_scale: 1
            fact: vehicle.mission
            title: fact.missionSize
            iconName: fact.icon
            warning: fact.missionSize<=0
            active: fact.modified || fact.progress>=0
            enabled: true
            onTriggered: {
                if(fact.missionSize<=0) fact.action.request.trigger()
                //else if(!fact.synced) fact.action.upload.trigger()
                fact.requestMenu()
            }
            onMenuRequested: fact.requestMenu()
        }
    }

    StackView {
        clip: true
        Layout.fillWidth: true
        Layout.margins: 4
        Layout.minimumHeight: currentItem?currentItem.implicitHeight:0
        property string mode: m.mode.text
        onModeChanged: {
            var modes
            var body
            switch(m.mode.value){
                default: modes=m.mode.enumStrings; break;
                case mode_EMG:
                    modes=["RPV","TAXI"]
                    body="EMG"
                    break
                case mode_RPV:
                    modes=["UAV","WPT"]
                    break
                case mode_UAV:
                    modes=["WPT","LANDING"]
                    break
                case mode_WPT:
                    modes=["STBY","LANDING"]
                    break
                case mode_HOME:
                    modes=["WPT","LANDING"]
                    break
                case mode_STBY:
                    modes=["WPT","LANDING"]
                    break
                case mode_TAXI:
                    modes=["TAKEOFF","EMG"]
                    break
                case mode_TAKEOFF:
                    modes=["WPT","STBY"]
                    break
                case mode_LANDING:
                    modes=["WPT","STBY"]
                    break
            }
            replace(null,"Mode.qml",{"modes": modes, "body": body },StackView.PushTransition)
        }
    }

    Pages {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 4
    }

    Ctr {
        Layout.fillWidth: true
        Layout.margins: 4
    }


}
