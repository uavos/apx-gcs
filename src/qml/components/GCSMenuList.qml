import QtQuick 2.7
import QtQml 2.2
import QtQml.Models 2.2

Item {
    property ObjectModel objModel
    property alias model: instantiator.model
    property alias delegate: instantiator.delegate
    property int listRow: ObjectModel.index+1 //objModel.children.indexOf(this)
    Instantiator {
        id: instantiator
        //asynchronous: true
        onObjectAdded: objModel.insert(listRow+index,object);
        onObjectRemoved: objModel.remove(listRow+index,1)
    }
}
