import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

ColumnLayout {
    id: consoleItem
    property int fontSize: 12
    spacing: 0
    anchors.margins: 3

    ListView {
        id: listView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignBottom
        Layout.preferredWidth: 300
        Layout.preferredHeight: 400
        implicitWidth: 300
        implicitHeight: 400

        model: apx.tools.DatalinkInspector.outModel
        delegate: DatalinkInspectorLine {
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
            }else scrollTimer.restart()
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

        //onCountChanged: positionViewAtIndex(count-1,ListView.End)

        //Behavior on contentY { enabled: ui.smooth; NumberAnimation { duration: 100 } }
        function scrollToEnd()
        {
            positionViewAtEnd()
            //flick(0,-height)
            //currentIndex=count-1
            //positionViewAtIndex(count-1,ListView.End)
        }
        /*footer: DatalinkInspectorExec {
            width: parent.width
            fontSize: consoleItem.fontSize
            onFocused: listView.positionViewAtEnd()
        }*/
        MouseArea {
            anchors.fill: parent
            onClicked: execControl.focusRequested()
        }
    }

    DatalinkInspectorExec {
        id: execControl
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignBottom
        Layout.maximumWidth: listView.width
        fontSize: consoleItem.fontSize
        onFocused: listView.scrollToEnd()
    }

}
