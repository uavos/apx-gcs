import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import GCS.FactSystem 1.0

import "."

Dialog {
    id: dialog
    modal: true
    title: fact.title //+ (fact.descr?"\n"+fact.descr:"")
    standardButtons: Dialog.Ok | Dialog.Cancel
    parent: menuPage
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    implicitWidth: itemSize*10
    onAboutToShow: {
        editor.text=fact.text;
        editor.selectAll();
        editor.forceActiveFocus();
    }
    onAccepted: fact.setValue(editor.text)
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 10
        Text {
            text: fact.descr + (fact.units?" ["+fact.units+"]":"")
            color: Material.hintTextColor
        }
        TextField {
            Layout.fillWidth: true
            id: editor
            selectByMouse: true
            onAccepted: dialog.accept()
        }
    }
    Connections {
        target: factButton
        onClicked: dialog.open()
    }
}
