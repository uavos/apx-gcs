import QtQuick 2.11
import QtQml 2.12

import QtQuick.Controls 2.4

import APX.Facts 1.0 as APX

MenuBar {
    id: menuBar

    Instantiator {
        model: apx.sysmenu.model
        Menu {
            id: menu
            title: modelData.title
            Instantiator {
                model: modelData.menu().model
                MenuItem {
                    text: modelData.title
                    onTriggered: modelData.trigger()
                    checkable: modelData.dataType===APX.Fact.Bool
                    checked: checkable?modelData.value:false
                    onToggled: modelData.value=checked

                }
                onObjectAdded: menu.insertItem(index,object)
                onObjectRemoved: menu.removeItem(object)
            }
        }
        onObjectAdded: menuBar.insertMenu(index,object)
        onObjectRemoved: menuBar.removeMenu(object)
    }
}
