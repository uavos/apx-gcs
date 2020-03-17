import QtQuick 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Apx.Common 1.0


Rectangle {
    id: control

    //implicitHeight: layout.implicitHeight
    //implicitWidth: layout.implicitWidth

    border.width: 0
    color: "#000"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 3

        DatalinkInspectorView {
            id: _view
            Layout.fillWidth: true
            Layout.fillHeight: true
            onPid: _filter.pid(text,color)
        }

        DatalinkInspectorFilter {
            id: _filter
            Layout.fillHeight: true
            onFilter: _view.filter(text,v)
        }


    }
}
