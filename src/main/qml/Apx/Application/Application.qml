import QtQuick 2.12

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

    //about dialog
    Component {
        id: c_about
        AboutDialog { }
    }



    //loader and temporary image
    showBgImage: loaderMain.opacity!=1
    Connections {
        target: application
        onLoadingFinished: {
            loaderMain.active=true
        }
        onAbout: {
            var c=c_about.createObject(application.window)
            c.closed.connect(c.destroy)
            c.open()
        }
    }
    Component.onCompleted: {
        raise()
        application.loadApp()
    }
}
