import QtQuick          2.3
import QtQuick.Controls 2.3
import QtLocation       5.3
import QtQml 2.12

Item {
    id: control

    property var fact: apx.tools?apx.tools.location:null
    property string text: fact?fact.status:""
    property int progress: fact?fact.progress:-1

    implicitWidth: 100
    implicitHeight: 10

    visible: progress>=0

    ProgressBar {
        anchors.fill: parent
        value: progress
        indeterminate: true
        opacity: ui.effects?0.9:1
        background.height: height
        contentItem.implicitHeight: parent.height
    }

    //running: visible
    Label {
        anchors.fill: parent
        color: "#60FFFFFF"
        text: control.text
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
