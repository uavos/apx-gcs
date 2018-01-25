import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

ComboBox {
    id: editor
    Layout.fillHeight: true

    model: fact.enumStrings
    Component.onCompleted: currentIndex=find(value)
    onActivated: {
        fact.setValue(textAt(index))
        parent.forceActiveFocus();
    }
    property string value: fact.text
    onValueChanged: currentIndex=find(value)
    Connections {
        target: listView
        onMovementStarted: editor.popup.close()
    }
}
