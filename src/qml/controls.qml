import QtQuick 2.2
import QtQml.Models 2.2
import QtQml 2.2
import "./components"

GCSMenu {
    //id: menu
    title: qsTr("Controls")

    fields: ObjectModel {
        GCSMenuField { title: qsTr("Commands"); pageMenu: "MenuCommands.qml"}
        GCSMenuField { title: qsTr("HDG"); page: "HDG.qml"}
        GCSMenuField { title: qsTr("Video"); page: "video.qml"}
        GCSMenuField {
            title: qsTr("Settings")
            fields: ObjectModel {
                GCSMenuField { title: qsTr("Shortcuts"); pageMenu: "Shortcuts.qml" }
            }
        }
    }

    /*model: ListModel {
        ListElement { title: qsTr("Commands"); subMenu: "MenuCommands.qml"}
        ListElement { title: qsTr("HDG"); page: "HDG.qml"}
        ListElement { title: qsTr("Video"); page: "video.qml"}
        ListElement {
            title: qsTr("Settings")
            contents: [
                ListElement { title: qsTr("Shortcuts"); subMenu: "Shortcuts.qml" }
            ]
        }
    }*/

    //fields:
    /*ObjectModel {
        id: objModel
        Rectangle { height: 30; width: 80; color: "red" }
        Item {id: menuList }
        Text {
            color: "white";
            height: 30;
            width: 80;
            text: ObjectModel.index+"/"+objModel.count
        }
        Rectangle { height: 10; width: 120; color: "blue" }
    }

    Instantiator {
        id: repeater
        property int listRow: menuList.ObjectModel.index+1 //objModel.children.indexOf(this)
        model: app.shortcuts.systemShortcuts
        delegate: Text {
            property string cmd: model.item.cmd
            id: field
            color: "white";
            height: 30;
            width: 80;
            text: index+": "+cmd
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if(mouse.modifiers==Qt.ShiftModifier)app.shortcuts.addNew();
                    else app.shortcuts.systemShortcuts.removeItem(item);
                }
            }
        }
        //onItemAdded: objModel.insert(listRow+index,item)
        //onItemRemoved: objModel.remove(listRow+index,1)
        onObjectAdded: objModel.insert(listRow+index,object)
        onObjectRemoved: objModel.remove(listRow+index,1)
    }*/
}
