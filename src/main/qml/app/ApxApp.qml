import QtQuick 2.12

import Apx.Common 1.0


AppWindow {
    id: control

    Loader {
        id: loaderMain
        active: false
        asynchronous: true
        source: "ApxAppMain.qml"
        anchors.fill: parent
        visible: false
        onLoaded: {
            visible=true
            var cname="ApxAppMainMenuBar"
            if(Qt.platform.os=="linux")cname+="Linux"
            else cname+="Macos"
            var component = Qt.createComponent(cname+".qml");
            if (component.status === Component.Ready) {
                var menu=component.createObject(control)
                menuBar=menu
            }
        }
        opacity: visible?1:0
        Behavior on opacity { PropertyAnimation { duration: 2000; easing.type: Easing.InQuart; } }
    }

    showBgImage: loaderMain.opacity!=1

    Connections {
        target: application
        onLoadingFinished: {
            loaderMain.active=true
        }
    }

    Component.onCompleted: raise()


    Timer {
        running: true
        interval: 300
        onTriggered: application.loadApp()
    }
}
