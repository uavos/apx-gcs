import QtQuick 2.6
import QtQuick.Controls 2.1

Item {
    id: root
    property string title
    property bool checkable: false
    property bool checked: false

    signal clicked()

    //onCheckedChanged: console.log(checked);


    //multi level
    default property alias contents: addItem.children
    Item { id: addItem }
    function appendItem(object)
    {
        object.parent=addItem
    }
    function removeItem(object)
    {
        //if(contents.contains(object))object.destroy()
        object.parent=root
        //delete object
        //object.destroy(1000)
    }

}
