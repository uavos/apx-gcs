import QtQuick          2.3
import QtQuick.Controls 2.3
import QtLocation       5.3
import QtQml 2.12

Item {
    id: mapBusy

    property var fact: apx.tools?apx.tools.Location:null
    property int value: fact?fact.requestCount:0

    visible: value>0

    ProgressBar {
        anchors.fill: parent
        value: 0
        indeterminate: true
        opacity: ui.effects?0.9:1
        background.height: height
        contentItem.implicitHeight: parent.height
    }

    //running: visible
    Label {
        anchors.fill: parent
        color: "#60FFFFFF"
        text: value.toFixed()
        font.bold: true
        horizontalAlignment: Qt.AlignRight
        verticalAlignment: Qt.AlignVCenter
        font.pixelSize: Math.max(8,height)
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
