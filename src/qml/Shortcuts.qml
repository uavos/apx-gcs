import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQml 2.2
import "./components"

GCSMenu {
    title: qsTr("Shortcuts")

    menu: GCSMenuItem {
        id: menuShortcuts
        property var list: Instantiator {
            model: app.shortcuts.items
            asynchronous: true
            GCSMenuItem {
                title: modelData.cmd
                height: itemSize //*2
                delegate: scElement
                //onClicked: mode.setValue(mode_RPV);
            }
            onObjectAdded: menuShortcuts.appendItem(object)
            onObjectRemoved: menuShortcuts.removeItem(object)
        }
    }


    Component {
        id: scElement
        Rectangle {
            width: parent.width
            height: root.itemSize
            border.width: 1
            radius: 4
            focus: true
            color: "transparent"
            border.color: activeFocus?"gray":"transparent"
        }
    }
}
