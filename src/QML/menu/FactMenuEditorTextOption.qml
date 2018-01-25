import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

FactMenuEditorOption {
    id: editor
    Layout.fillWidth: true
    Layout.preferredWidth: factButton.width*0.7

    editable: true
    contentItem: TextInput {
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignRight
        font.pixelSize: editorFontSize
        color: activeFocus?Style.cValueTextEdit:Style.cValueText
        font.family: font_condenced
        text: fact.text
        selectByMouse: true
        onEditingFinished: {
            fact.setValue(text);
            editor.parent.forceActiveFocus();
        }
        onActiveFocusChanged: if(activeFocus)selectAll();
    }
    background: Item {}
}
