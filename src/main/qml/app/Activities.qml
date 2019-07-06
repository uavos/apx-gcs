import QtQuick 2.3
import QtQuick.Layouts 1.3

import Apx.Common 1.0

Item {
    property int btnSize: height/container.children.length-container.spacing

    implicitWidth: Math.min(48,btnSize)*ui.scale

    ColumnLayout {
        id: container
        width: parent.width
        spacing: 1
        /*CleanButton {
            Layout.fillWidth: true
            implicitHeight: width
            iconName: "airplane"
            highlighted: true
        }
        CleanButton {
            Layout.fillWidth: true
            implicitHeight: width
            iconName: "map"
        }
        CleanButton {
            Layout.fillWidth: true
            implicitHeight: width
            iconName: "car-connected"
        }
        CleanButton {
            Layout.fillWidth: true
            implicitHeight: width
            iconName: "video"
        }
        CleanButton {
            Layout.fillWidth: true
            implicitHeight: width
            iconName: "settings"
        }
        CleanButton {
            Layout.fillWidth: true
            implicitHeight: width
            iconName: "playlist-check"
        }*/
    }

    Component.onCompleted: {
        for(var i in groundControl.states){
            var a=groundControl.states[i]
            buttonC.createObject(container,{"stateItem":a, "name":a.name})
        }
    }

    Component {
        id: buttonC
        CleanButton {
            property var stateItem
            property string name
            Layout.fillWidth: true
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
