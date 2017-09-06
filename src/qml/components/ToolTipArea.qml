import QtQuick 2.2

MouseArea {
    //property alias tip: tip
    //propagateComposedEvents: true
    //preventStealing: true
    property string text    // tip.text
    //property alias hideDelay: hideTimer.interval
    property alias showDelay: showTimer.interval
    property bool tipVisible: false
    id: mouseArea
    acceptedButtons: Qt.NoButton
    anchors.fill: parent
    hoverEnabled: true
    function show(){
        visible = true;
        mandala.toolTip(text);
    }
    Timer {
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
    }
    /*ToolTip{
        id:tip
    }*/
}

