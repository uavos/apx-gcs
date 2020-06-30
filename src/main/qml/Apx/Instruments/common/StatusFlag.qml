import QtQuick 2.2
import "."

BlinkingText {
    id: control

    text: fact.title.toUpperCase()

    property real status: fact.value
    property real status_warning: 2
    property real status_show: status_warning
    property real status_failure: status_warning+1

    property int status_reset: -1

    property int type_default: CleanText.White
    property int type_warning: CleanText.Yellow

    font: font_condenced

    width: height/0.35

    show: ui.test || status >= status_show
    blinking: failure

    property bool warning: status >= status_warning
    property bool failure: status >= status_failure


    type: failure
          ? CleanText.Red
          : warning
            ? type_warning
            : type_default

    visible: ui.test || show

    MouseArea {
        anchors.fill: parent
        enabled: status_reset >= 0 && show
        onClicked: fact.value = status_reset
    }
}

