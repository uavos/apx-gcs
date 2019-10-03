import QtQuick          2.3
import QtQuick.Controls 2.3
import QtLocation       5.3
import QtQml 2.12

Item {
    id: mapBusy

    property var fact: apx.tools?apx.tools.location:null
    property int value: fact?fact.requestCount:0

    implicitWidth: 100
    implicitHeight: 10

    ProgressBar {
        anchors.fill: parent
        visible: value>=0
        value: -1
        indeterminate: true
        opacity: ui.effects?0.9:1
        background.height: height
        contentItem.implicitHeight: parent.height
    }

    //running: visible
    Label {
        anchors.fill: parent
        visible: value>=0
        color: "#60FFFFFF"
        text: value.toFixed()
        font.bold: true
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        font.pixelSize: Math.max(8,height)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            fact.abort();
        }
    }
}
