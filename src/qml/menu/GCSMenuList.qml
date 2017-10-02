import QtQuick 2.7
import QtQml 2.2
import "."

Item {
    property GCSMenuModel objModel
    property alias model: instantiator.model
    property alias delegate: instantiator.delegate
    property int listRow: GCSMenuModel.index+1 //objModel.children.indexOf(this)
    Instantiator {
        id: instantiator
        //asynchronous: true
        onObjectAdded: objModel.insert(listRow+index,object);
        onObjectRemoved: objModel.remove(listRow+index,1)
    }
}
