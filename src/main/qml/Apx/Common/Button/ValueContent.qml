import QtQuick 2.12

Item {
    id: control
    property Component iconC
    property Component textC
    property Component valueC

    implicitWidth: _titleRow.implicitWidth + _valueRow.implicitWidth + 2
    implicitHeight: 24

    Row {
        id: _titleRow
        spacing: 0
        anchors.fill: parent
        anchors.rightMargin: Math.min(_valueRow.implicitWidth, _valueRow.width)
        clip: true

        // icon
        Loader {
            id: _icon
            height: parent.height
            width: item?height:0
            sourceComponent: iconC
        }

        // title
        Loader {
            id: _title
            height: parent.height
            sourceComponent: textC
        }
    }
    Row {
        id: _valueRow
        spacing: 0
        anchors.fill: parent
        anchors.leftMargin: (_icon.item || _title.item)?parent.width*0.1:0
        clip: true

        layoutDirection: Qt.RightToLeft
        Row {
            height: parent.height

            // value
            Loader {
                id: _value
                height: parent.height
                sourceComponent: valueC
            }
        }
    }
}
