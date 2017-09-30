import QtQuick 2.6
import QtQuick.Controls 2.1

Item {
    id: menuItemTemplate
    property string title
    property bool separator: false
    property var busy: false
    property bool showBusy: !delegate

    property bool checkable: false
    property bool checked: false

    property bool enabled: true

    property Component delegate

    signal clicked()
    signal toggled()

    property bool show: true

    property string page
    property string subMenu

    property var itemData

    property var menu   //sub menu items

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
        object.parent=menuItemTemplate
        //object.destroy()
        //object.destroy(1000)
    }

    function appendItems(items)
    {
        for(var i=0;i<items.length;i++)appendItem(items.at(i));
    }


}
