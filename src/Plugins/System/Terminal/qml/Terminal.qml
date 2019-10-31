import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import APX.Facts 1.0

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: 300
    implicitHeight: 250

    readonly property int margins: 3
    property int fontSize: 12

    //property var terminal: apx.tools.terminal

    ColumnLayout {
        spacing: 0
        anchors.margins: 3
        anchors.fill: parent


        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredWidth: 300
            Layout.preferredHeight: 400

            clip: true

            model: application.notifyModel
            delegate: TerminalLine {
                width: listView.width
                fontSize: control.fontSize
                text: model.text
                subsystem: model.subsystem
                source: model.source
                type: model.type
                options: model.options
                fact: model.fact
                timestamp: model.timestamp
            }

            add: Transition {
                id: transAdd
                enabled: ui.smooth
                NumberAnimation {
                    properties: "x";
                    from: -listView.width;
                    duration: transAdd.ViewTransition.item.source==AppNotify.FromInput?0:150
                    easing.type: Easing.OutCubic
                }
            }


            ScrollBar.vertical: ScrollBar {
                width: 6
                active: !listView.atYEnd
            }
            boundsBehavior: Flickable.StopAtBounds
            readonly property bool scrolling: dragging||flicking
            property bool stickEnd: false
            onAtYEndChanged: {
                if(atYEnd)stickEnd=scrolling
                else if(stickEnd)scrollToEnd()
                else if(!(scrolling||atYEnd||scrollTimer.running)) scrollToEnd()//scrollTimerEnd.start()
            }
            onScrollingChanged: {
                //console.log(scrolling)
                if(scrolling && (!atYEnd)){
                    scrollTimer.stop()
                    stickEnd=false
                }//else scrollTimer.restart()
            }
            Timer {
                id: scrollTimer
                interval: 2000
                onTriggered: scrollTimerEnd.start()
            }
            Timer {
                id: scrollTimerEnd
                interval: 1
                onTriggered: if(!listView.scrolling)listView.scrollToEnd()
            }

            Component.onCompleted: scrollToEnd()

            focus: false
            keyNavigationEnabled: false

            footer: TerminalExec {
                width: parent.width
                fontSize: control.fontSize
                onFocused: listView.scrollToEnd()
            }

            function scrollToEnd()
            {
                positionViewAtEnd()
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listView.footerItem.focusRequested()
                    listView.scrollToEnd()
                }
            }
        }
    }
}
