import QtQuick 2.5

MapText {
    id: button

    property bool active: false
    property real defaultOpacity: ui.effects?0.7:1

    //exported
    signal clicked()
    signal doubleClicked()
    signal pressAndHold()
    signal menuRequested()

    property bool hovered: mouseArea.containsMouse

    //internal
    square: true

    opacity: (enabled && (!active) && (!hovered))?defaultOpacity:1
    Behavior on opacity { enabled: ui.smooth; NumberAnimation {duration: 200; } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: button.enabled
        hoverEnabled: enabled
        cursorShape: enabled?Qt.PointingHandCursor:Qt.ArrowCursor
        onClicked: button.clicked()
        onDoubleClicked: { button.doubleClicked(); button.menuRequested(); }
        onPressAndHold: { button.pressAndHold(); button.menuRequested(); }
    }
}
