import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Qt.labs.settings 1.0
import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import Apx.Common 1.0
import Apx.Menu 1.0
import APX.Facts 1.0

Dialog {
    id: editor

    property var fact

    property string value: fact? fact.value:""

    visible: true
    title: qsTr("Script editor")
    standardButtons: Dialog.Ok | Dialog.Apply | Dialog.Cancel
    onClosed: destroy()
    modal: false
    closePolicy: Popup.NoAutoClose

    x: Math.max(0,(parent.width-width)/2)
    y: Math.max(0,(parent.height-height)/2)

    padding: 0

    onApplied: {
        fact.value=editedText
    }
    onAccepted: {
        fact.value=editedText
    }
    onFactChanged: if(!fact)reject()

    property alias editedText: textArea.text

    //contentWidth: layout.implicitWidth
    contentHeight: layout.implicitHeight
    contentItem:
    ColumnLayout {
        id: layout
        ScrollView {
            Layout.topMargin: header.height
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: textArea
                selectByMouse: true
                selectByKeyboard: true
                text: value
                textFormat: TextEdit.PlainText
                wrapMode: Text.NoWrap
                font.family: font_fixed
                Material.background: "#000"
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.bottomMargin: footer.height
        }
    }

}
