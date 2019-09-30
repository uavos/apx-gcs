import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

ColumnLayout {
    id: consoleItem
    property int fontSize: 12
    spacing: 0
    anchors.margins: 3

    property var terminal: apx.tools.terminal

    ListView {
        id: listView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignBottom
        Layout.preferredWidth: 300
        Layout.preferredHeight: 400
        implicitWidth: 300
        implicitHeight: 400

        model: terminal.outModel
        delegate: TerminalLine {
            width: listView.width
            line: model
            fontSize: consoleItem.fontSize
        }

        /*add: Transition {
            enabled: ui.smooth
            //NumberAnimation { properties: "x,y"; duration: 1000 }
            NumberAnimation { properties: "x"; from: -listView.width; duration: 50 }
        }*/


        ScrollBar.vertical: ScrollBar {
            width: 6
            active: !listView.atYEnd
            //policy: ScrollBar.AlwaysOn
        }
        //snapMode: ListView.SnapOneItem
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


        focus: false
        keyNavigationEnabled: false

        footer: TerminalExec {
            width: parent.width
            fontSize: consoleItem.fontSize
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

    /*TerminalExec {
        id: execControl
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignBottom
        Layout.maximumWidth: listView.width
        fontSize: consoleItem.fontSize
        onFocused: listView.scrollToEnd()
    }*/

}
