import QtQuick 2.6
import QtQuick.Controls 2.1

import Apx.Controls 1.0

import "../common"

Item{

    implicitWidth: 800
    implicitHeight: 400


    opacity: ui.effects?(loaded?1:0.3):1
    property bool loaded: comm.status === Loader.Ready
                          && pfd.status === Loader.Ready
                          && nav.status === Loader.Ready
    Behavior on opacity { enabled: ui.smooth; PropertyAnimation {duration: 500} }

    Loader {
        id: comm
        asynchronous: ui.smooth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: apx.limit(parent.height*0.05,10,width/32)
        source: "../comm/Comm.qml"
    }

    Loader {
        id: pfd
        asynchronous: ui.smooth
        anchors.fill: parent
        anchors.topMargin: comm.height
        anchors.bottomMargin: apx.limit(comm.height,20,comm.height)
        source: "../pfd/Pfd.qml"
    }

    Rectangle {
        id: btm_sep
        border.width: 1
        border.color: "#770"
        height: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: pfd.bottom
    }

    Loader {
        id: nav
        asynchronous: ui.smooth
        anchors.top: btm_sep.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        source: "../nav/Nav.qml"
    }

    /*Btn {
        anchors.top: pfd.top
        anchors.topMargin: 3
        anchors.right: pfd.right
        anchors.rightMargin: 3
        height: 32
        width: height
        text: qsTr("+")
        onClicked: popupMenu.open()
    }*/


}
