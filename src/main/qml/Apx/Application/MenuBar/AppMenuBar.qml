import QtQuick 2.12
import QtQml 2.12

Loader {
    asynchronous: true
    source: {
        var cname="MenuBar"
        if(Qt.platform.os=="linux")cname+="Linux"
        else cname+="Macos"
        return cname+".qml"
    }

    /*ListModel {
        id: model
    }
    Instantiator {
        model: apx.vehicles.select.model
        ListElement {
            text: modelData.title
            onTriggered: modelData.trigger()
            checked: modelData.active
        }
        onObjectAdded: model.insert(index,object)
        onObjectRemoved: model.remove(object)
    }*/
}
