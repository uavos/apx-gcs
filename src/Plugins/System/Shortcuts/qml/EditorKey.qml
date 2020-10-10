import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3

RowLayout {
    id: editor
    //Layout.fillWidth: true
    Shortcut {
        id: scText
        enabled: false
        sequence: (s.length>1 && s.endsWith("+"))?s.substr(0,s.length-1):s
        property alias s: _textInput.text
    }
    Text {
        Layout.fillHeight: true
        verticalAlignment: Qt.AlignVCenter
        font.family: font_condenced
        font.pixelSize: control.valueSize
        color: Material.color(Material.Green)
        text: scText.nativeText
    }
    TextField {
        id: _textInput
        focus: true
        //Layout.alignment: Qt.AlignRight
        //Layout.fillWidth: true
        Layout.fillHeight: true
        //anchors.verticalCenter: parent.verticalCenter
        topPadding: 0
        bottomPadding: 0
        //height: MenuStyle.itemSize
        font.family: font_condenced
        font.pixelSize: control.valueSize
        color: _textInput.activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
        placeholderText: qsTr("Key Sequence")
        text: fact.text
        hoverEnabled: true
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignRight
        selectByMouse: true
        persistentSelection: true
        background: Rectangle {
            anchors.fill: parent
            border.width: 0
            color: "#000"
            opacity: 0.3
        }
        onActiveFocusChanged: {
            apx.settings.interface.shortcuts.blocked.setValue(activeFocus);
            if(activeFocus && _textInput.selectedText===""){
                _textInput.selectAll();
            }
        }
        onEditingFinished: {
            apx.settings.interface.shortcuts.blocked.setValue(false);
            if(modelData) fact.setValue(text)
        }
        //Keys.onEscapePressed: editor.parent.forceActiveFocus();
        Keys.onPressed: {
            //console.log("key: "+event.key+" text: "+event.text)
            event.accepted=true
            _textInput.remove(_textInput.selectionStart,_textInput.selectionEnd);
            var s=apx.settings.interface.shortcuts.keyToPortableString(event.key,event.modifiers);
            var i=_textInput.cursorPosition;
            if(_textInput.text.endsWith('+'))i=_textInput.text.length;
            _textInput.insert(i,s);
            if(!_textInput.text.endsWith('+'))_textInput.selectAll();
        }
    }
}
