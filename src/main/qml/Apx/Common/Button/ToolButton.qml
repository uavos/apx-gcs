import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import ".."

IconButton {
    id: control


    default property alias contents: _row.children

    property alias contentsItem: _row

    contentItem: Row {
        id: _row
        spacing: 3
    }

    Component.onCompleted: {
        addItem(iconItem)
        addItem(textItem)
    }

    function addItem(item)
    {
        item.parent = _row
        item.height = Qt.binding(function(){return _row.height})
    }
}
