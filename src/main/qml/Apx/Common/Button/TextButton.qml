import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

ButtonBase {
    id: control

    property bool showText: true

    property color textColor: Material.primaryTextColor
    property color disabledTextColor: Material.hintTextColor

    property color currentTextColor: enabled?textColor:disabledTextColor

    property real textScale: 0.6
    readonly property int textSize: Math.max(7, control.height * textScale - 2)

    font.pixelSize: textSize

    readonly property Item textItem: Text {
        visible: showText && text
        font: control.font
        text: control.text
        color: control.currentTextColor

        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }


    contentItem: textItem
}
