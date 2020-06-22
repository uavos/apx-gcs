import QtQuick 2.2
import "."

CleanText {
    id: control

    property bool blinking: false
    property bool blinkingFG: true

    property int interval: 300

    readonly property bool hide_blink: !(ui.test || (show && blink))

    hide_bg: hide_blink
    hide_fg: blinkingFG ? hide_blink : !(show || ui.test)

    property bool blink: true
    onBlinkingChanged: blink=true

    SequentialAnimation on visible {
        running: blinking
        loops: Animation.Infinite
        PropertyAction { target: control; property: "blink"; value: true }
        PauseAnimation { duration: control.interval }
        PropertyAction { target: control; property: "blink"; value: false }
        PauseAnimation { duration: control.interval }
    }

}

