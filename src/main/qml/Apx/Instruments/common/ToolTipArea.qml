import QtQuick 2.2
import QtQuick.Controls 2.3

MouseArea {
    id: mouseArea
    //property alias tip: tip
    //propagateComposedEvents: true
    //preventStealing: true
    property string text    // tip.text
    //property alias hideDelay: hideTimer.interval
    //property alias showDelay: showTimer.interval
    //property bool tipVisible: false
    acceptedButtons: Qt.NoButton
    anchors.fill: parent
    hoverEnabled: true
    /*function show(){
        visible = true;
        apx.toolTip(text);
    }*/
    /*Timer {
        id:showTimer
        interval: 1000
        running: mouseArea.containsMouse && !tipVisible && text.length>0
        onTriggered: show();
    }
    Timer {
        id:hideTimer
        interval: 100
        running: !mouseArea.containsMouse && tipVisible
        onTriggered: tipVisible=false;  //tip.hide();
    }*/
    /*ToolTip{
        id:tip
    }*/
    ToolTip {
        delay: 1000
        timeout: 5000
        visible: containsMouse
        text: mouseArea.text
    }
}

