import QtQuick
import QtQuick.Controls

SpinBox {
    id: spin

    implicitHeight: 26
    editable: false
    from: 1
    to: 50
    stepSize: 1
    value: 10

    // value/10: real step 0.1..5.0 deg
    readonly property real realStep: value / 10

    textFromValue: function (value, locale) {
        return (value / 10).toFixed(1) + "°"
    }
    valueFromText: function (text, locale) {
        return Math.round(parseFloat(text) * 10)
    }
    contentItem: TextInput {
        text: spin.textFromValue(spin.value, spin.locale)
        readOnly: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#e0e0e0"
        font.pixelSize: 13
        selectByMouse: false
    }
}
