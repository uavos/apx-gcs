import QtQuick 2.3
import QtQuick.Layouts 1.3

import Apx.Common 1.0

ColumnLayout {
    id: control
    spacing: 1

    property int buttonSize: 44*ui.scale

    Component.onCompleted: {
        for(var i in groundControl.states){
            var a=groundControl.states[i]
            buttonC.createObject(control,{"stateItem":a, "name":a.name})
        }
    }

    Component {
        id: buttonC
        CleanButton {
            property var stateItem
            property string name
            //Layout.fillWidth: true
            implicitWidth: buttonSize
            implicitHeight: width

            iconSize: 0.5
            iconName: stateItem.icon
            checked: mainState===name
            autoExclusive: true
            checkable: true
            highlighted: checked
            onCheckedChanged: {
                if(checked){
                    groundControl.state=name
                }
            }
        }
    }
}
