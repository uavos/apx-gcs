import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

ComboBox {
    id: editor
    Layout.fillHeight: true

    spacing: 0
    topPadding: 0
    bottomPadding: 0
    background.y: 2
    background.height: height - 4


    model: fact.enumStrings
    Component.onCompleted: {
        //console.log("cmp")
        currentIndex=find(value)
    }
    onActivated: {
        fact.setValue(textAt(index))
        parent.forceActiveFocus();
    }
    property string value: fact.text
    onValueChanged: {
        //console.log("chg")
        currentIndex=find(value)
    }
    Connections {
        target: listView
        onMovementStarted: {
            editor.popup.close()
        }
    }
}
