import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtQml 2.12

Rectangle {
    id: control
    implicitHeight: textInput.contentHeight+4
    implicitWidth: Math.max(textInput.contentWidth+4,height*2)
    radius: 3
    color: "#50000000"
    border.width: 0

    TextInput {
        id: textInput
        anchors.centerIn: parent
        clip: true
        focus: true

        width: control.width-4

        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter

        font.family: font_condenced
        font.pixelSize: fontSize(bodyHeight*valueSize)

        color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
        text: fact.text

        activeFocusOnTab: true
        selectByMouse: true

        onEditingFinished: {
            fact.setValue(text);
            factButton.focusFree();
        }
        onActiveFocusChanged: {
            if(activeFocus)selectAll();
        }
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
        textInput.forceActiveFocus()
    }
}
