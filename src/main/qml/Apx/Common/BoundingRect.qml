import QtQuick 2.5;

Rectangle {
    z: 9999

    property var item

    anchors.fill: item?null:parent
    color: "transparent"
    border.width: 2
    border.color: "#fff"
    opacity: 0.3

    Component.onCompleted: {
        if(!item)return
        console.log(item)

        x = Qt.binding(function(){return itemRect().x})
        y = Qt.binding(function(){return itemRect().y})
        width = Qt.binding(function(){return item.width})
        height = Qt.binding(function(){return item.height})
    }
    function itemRect()
    {
        return parent.mapFromItem(item.parent, item.x, item.y)
    }
}
