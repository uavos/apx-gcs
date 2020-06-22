import QtQuick 2.2
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import "."

StatusFlag {
    id: control

    property string title: fact.name.toUpperCase()
    property real value: fact.value

    property real precision: 0

    blinking: failure
    warning: false
    failure: false

    text: value.toFixed(precision)

    width: implicitWidth

    type_default: CleanText.Clean
    type_warning: CleanText.Normal
    show: true

    font: font_narrow

    blinkingFG: false

    prefixItems: [
        Text {
            id: textItem
            Layout.fillHeight: true
            verticalAlignment: Text.AlignTop
            text: control.title
            font.pixelSize: Math.max(4, control.height*0.6)
            font.family: font_narrow
            color: "#80DEEA"
        }
    ]
}

