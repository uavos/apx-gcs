import QtQuick 2.6
import QtQuick.Controls 2.1
import "qrc:///pfd"
import "qrc:///components"
import "qrc:///menu"

Item{
    id: window
    //Material.theme: Material.Dark

    opacity: loaded?1:0.3
    property bool loaded: comm.status === Loader.Ready &&
                          pfd.status === Loader.Ready &&
                          nav.status === Loader.Ready
    Behavior on opacity { enabled: app.settings.smooth.value; PropertyAnimation {duration: 500} }

    GCSMenuPopup {
        id: popupMenu
        menu: MenuCommands {  }
        //source: "menu/MenuCommands.qml"
    }

    Loader {
        id: comm
        asynchronous: app.settings.smooth.value
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: sys.limit(parent.height*0.05,10,width/32)
        source: "qrc:///comm/Comm.qml"
    }

    Loader {
        id: pfd
        asynchronous: app.settings.smooth.value
        anchors.fill: parent
        anchors.topMargin: comm.height
        anchors.bottomMargin: sys.limit(comm.height,20,comm.height)
        source: "qrc:///pfd/Pfd.qml"
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
        asynchronous: app.settings.smooth.value
        anchors.top: btm_sep.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        source: "qrc:///nav/Nav.qml"
    }

    Btn {
        anchors.top: pfd.top
        anchors.topMargin: 3
        anchors.right: pfd.right
        anchors.rightMargin: 3
        height: 32
        width: height
        text: qsTr("+")
        onClicked: popupMenu.open()
    }


}
