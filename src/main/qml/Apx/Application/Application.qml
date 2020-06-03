import QtQuick 2.12
import QtQuick.Controls 2.12

import Apx.Menu 1.0

Control {

    // load sequence
    Timer {
        interval: 1
        running: true
        onTriggered: application.loadApp()
    }

    Connections {
        target: application
        function onLoadingFinished()
        {
            loaderMain.active=Qt.binding(function(){return !apx.value})
        }
        function onAbout()
        {
            var c=c_about.createObject(application.window)
            c.closed.connect(c.destroy)
            c.open()
        }
        /*onClosing: {
            loaderMain.visible=false
            loaderMain.active=false
        }*/
    }

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
            application.registerUiComponent(item, "main")
        }
        opacity: visible?1:0
        Behavior on opacity { PropertyAnimation { duration: 2000; easing.type: Easing.InQuart; } }
    }

    //fact menu dispatcher
    Connections {
        target: apx
        function onFactTriggered(fact,opts)
        {
            Menu.show(fact,opts)
        }
    }

    //about dialog
    Component {
        id: c_about
        AboutDialog { }
    }


    //loader and temporary image
    property bool showBgImage: loaderMain.opacity!=1

    property string logText
    property string loaderText: apx.value?apx.text:logText
    background: Rectangle {
        border.width: 0
        color: "#080808"
        Loader {
            anchors.centerIn: parent
            active: showBgImage
            sourceComponent: Component {
                Image {
                    source: "qrc:///icons/uavos-logo.ico"
                    Text {
                        id: _text
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.bottom
                        color: "#888"
                        text: loaderText
                    }
                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: _text.bottom
                    }
                }
            }
        }
    }
    Connections {
        target: application.appLog
        function onConsoleMessage(msg)
        {
            logText = msg
        }
    }
}
