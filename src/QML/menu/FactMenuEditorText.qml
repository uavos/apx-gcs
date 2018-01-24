import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2
import GCS.FactSystem 1.0
import "."

TextInput {
    id: textInput

    property var fact

    Layout.fillHeight: true
    Layout.minimumWidth: itemSize*2

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.family: font_condenced
    font.pixelSize: height*0.6
    color: activeFocus?colorValueTextEdit:colorValueText
    text: fact.text

    selectByMouse: true
    onEditingFinished: {
        fact.setValue(text);
        parent.forceActiveFocus();
    }
    onActiveFocusChanged: {
        if(activeFocus)selectAll();
        //factItemButton.enabled=!activeFocus
    }
    Rectangle {
        visible: fact.enabled
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: -itemSize*0.05
        anchors.rightMargin: anchors.leftMargin
        anchors.verticalCenter: parent.verticalCenter
        height: itemSize*0.8
        radius: 3
        color: "transparent"
        border.width: 1
        border.color: parent.color
        opacity: 0.3
    }
    Connections {
        target: factItemButton
        onClicked: dialog.open()
    }
    Dialog {
        id: dialog
        modal: true
        title: fact.title + (fact.descr?" ("+fact.descr+")":"")
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
        TextField {
            id: editor
            anchors.left: parent.left
            anchors.right: parent.right
            selectByMouse: true
            placeholderText: fact.descr
            onAccepted: dialog.accept()
        }
    }
}
