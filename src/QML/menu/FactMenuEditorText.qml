import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

TextInput {
    Layout.minimumWidth: itemSize*2
    Layout.maximumHeight: editorFontSize*2

    horizontalAlignment: Text.AlignRight

    font.family: font_condenced
    font.pixelSize: editorFontSize

    color: activeFocus?Style.cValueTextEdit:Style.cValueText
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
        z: parent.z-1
        visible: fact.enabled
        anchors.fill: parent
        anchors.margins: -itemSize*0.05
        radius: 3
        color: "#000"
        border.width: 1
        border.color: Style.cValueFrame
        opacity: 0.3
    }

    FactMenuEditorDialog { }
}
