import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
//import GCS.Map          1.0

BusyIndicator {
    id: mapBusy

    property int value: 0 //(typeof mapLoader != "undefined")?mapLoader.requestCount:0


    Timer {
        id: bindTimer
        interval: 1000
        running: true
        repeat: false
        onTriggered: {
            if(typeof mapLoader != "undefined"){
                value=Qt.binding(function() { return mapLoader.requestCount });
            }else{
                value=0;
                start();
            }
        }
    }

    visible: value>0
    running: visible
    Label {
        anchors.centerIn: parent
        color: "#555"
        text: value.toFixed()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if(typeof mapLoader != "undefined"){
                mapLoader.abort();
            }
        }
    }
}
