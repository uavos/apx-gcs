import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQml 2.12

EditorOption {
    id: control

    editable: true
    contentItem: TextInput {
        id: textInput
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignRight
        font.family: font_condenced
        font.pixelSize: fontSize(bodyHeight*valueSize)
        color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
        text: fact.text
        selectByMouse: true
        onEditingFinished: {
            fact.setValue(text);
            factButton.focusFree();
        }
        onActiveFocusChanged: if(activeFocus)selectAll();
    }
    background: //Item {}
    Rectangle {
        z: parent.z-1
        visible: fact.enabled
        anchors.right: contentItem.right
        anchors.verticalCenter: contentItem.verticalCenter
        anchors.rightMargin: -10
        width: contentItem.contentWidth-anchors.rightMargin*2
        height: contentItem.height
        radius: 3
        color: "#000"
        border.width: 0
        opacity: 0.3
    }

    Connections {
        target: factButton
        onFocusRequested: checkFocusRequest()
    }
    Component.onCompleted: checkFocusRequest()
    function checkFocusRequest()
    {
        if(!factButton.bFocusRequest)return
        factButton.bFocusRequest=false
        control.forceActiveFocus()
    }

}
