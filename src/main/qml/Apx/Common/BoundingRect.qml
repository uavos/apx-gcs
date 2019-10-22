import QtQuick 2.5;

Rectangle {
    id: rect
    z: 9999

    property var item

    anchors.fill: item?null:parent
    color: "transparent"
    border.width: 2
    border.color: "#fff"
    opacity: 0.3

    Component.onCompleted: {
        console.warn(rect, item)
        if(!item)return

        x = Qt.binding(function(){return itemRect().x})
        y = Qt.binding(function(){return itemRect().y})
        width = Qt.binding(function(){return item.width})
        height = Qt.binding(function(){return item.height})
    }
    function itemRect()
    {
        var p=item.parent.mapToGlobal(item.x, item.y)
        return parent.mapFromGlobal(p.x,p.y)
    }
}
