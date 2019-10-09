import QtQuick 2.12

import Apx.Common 1.0
import Apx.Menu 1.0


AppWindow {
    id: control

    //load main layout
    Loader {
        id: loaderMain
        anchors.fill: parent
        active: false
        asynchronous: true
        source: typeof(qmlMainFile)!='undefined'?qmlMainFile:"GroundControl.qml"
        visible: false //hide to show BG image
        onLoaded: {
            visible=true
            //menu bar
            var cname="MenuBar"
            if(Qt.platform.os=="linux")cname+="Linux"
            else cname+="Macos"
            var component = Qt.createComponent(cname+".qml");
            if (component.status === Component.Ready) {
                var menu=component.createObject(control)
                menuBar=menu
            }
            //register and broadcast ui component availability for plugins
            application.registerUiComponent(item,"main")
        }
        opacity: visible?1:0
        Behavior on opacity { PropertyAnimation { duration: 2000; easing.type: Easing.InQuart; } }
    }

    //fact menu dispatcher
    Connections {
        target: apx
        onFactTriggered: {
            Menu.show(fact,opts)
        }
    }


    //loader and temporary image
    showBgImage: loaderMain.opacity!=1
    Connections {
        target: application
        onLoadingFinished: {
            loaderMain.active=true
        }
    }
    Component.onCompleted: {
        raise()
        application.loadApp()
    }
}
