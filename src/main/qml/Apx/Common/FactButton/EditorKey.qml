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
        property alias s: control.text
    }
    Text {
        //anchors.verticalCenter: parent.verticalCenter
        //Layout.fillWidth: true
        //anchors.leftMargin: font.pixelSize
        verticalAlignment: Qt.AlignVCenter
        font.family: font_condenced
        font.pixelSize: fontSize(bodyHeight*valueSize)
        color: Material.color(Material.Green)
        text: scText.nativeText
    }
    TextField {
        id: control
        focus: true
        Layout.alignment: Qt.AlignRight
        //Layout.fillWidth: true
        //Layout.fillHeight: true
        //anchors.verticalCenter: parent.verticalCenter
        topPadding: 0
        bottomPadding: 0
        //height: MenuStyle.itemSize
        font.family: font_condenced
        font.pixelSize: fontSize(bodyHeight*valueSize)
        color: control.activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
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
            if(activeFocus && control.selectedText===""){
                control.selectAll();
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
            control.remove(control.selectionStart,control.selectionEnd);
            var s=apx.settings.interface.shortcuts.keyToPortableString(event.key,event.modifiers);
            var i=control.cursorPosition;
            if(control.text.endsWith('+'))i=control.text.length;
            control.insert(i,s);
            if(!control.text.endsWith('+'))control.selectAll();
        }
    }
}
