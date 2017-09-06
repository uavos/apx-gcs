import QtQuick 2.2
import QtQuick.Controls 1.2

Menu {
    id: menu
    Instantiator {
        model: actions
        MenuItem {
            action: model.text
            //onTriggered: actions
        }
        onObjectAdded: menu.insertItem(index, object)
        onObjectRemoved: menu.removeItem(object)
    }
}
