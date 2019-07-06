import QtQuick 2.5;
import QtQuick.Layouts 1.3

import QtQuick.Controls 2.4

import Apx.Common 1.0

RowLayout {
    id: control

    //property var activities: []
    /*Rectangle {
        //anchors.fill: control
        border.width: 1
        border.color: "#88f"
        color: "transparent"
        width: control.implicitWidth
        height: control.implicitHeight
        x: 0
        y: 0
    }*/

    ButtonGroup {
        id: group
    }

    Component.onCompleted: {
        for(var i in groundControl.states){
            var a=groundControl.states[i]
            buttonC.createObject(control,{"stateItem":a})
        }
    }

    Component {
        id: buttonC
        CleanButton {
            property var stateItem
            iconName: stateItem.icon
            //checked: groundControl.state===stateItem.name
            implicitHeight: parent.height
            ButtonGroup.group: group
            checkable: true
            onCheckedChanged: {
                if(checked){
                    groundControl.state=stateItem.name
                }
            }
            Component.onCompleted: checked=groundControl.state===stateItem.name
        }
    }

    /*CleanButton {
        implicitHeight: parent.height
        iconName: "airplane"
        ButtonGroup.group: group
        checkable: true
        checked: true
    }
    CleanButton {
        implicitHeight: parent.height
        iconName: "map"
        ButtonGroup.group: group
        checkable: true
    }
    CleanButton {
        implicitHeight: parent.height
        iconName: "car-connected"
        ButtonGroup.group: group
        checkable: true
    }
    CleanButton {
        implicitHeight: parent.height
        iconName: "video"
        ButtonGroup.group: group
        checkable: true
    }
    CleanButton {
        implicitHeight: parent.height
        iconName: "settings"
        ButtonGroup.group: group
        checkable: true
    }
    CleanButton {
        implicitHeight: parent.height
        iconName: "playlist-check"
        ButtonGroup.group: group
        checkable: true
    }*/


}
