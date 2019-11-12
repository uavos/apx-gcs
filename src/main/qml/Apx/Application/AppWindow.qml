import QtQuick 2.3

//import QtQuick.Controls 1.3
import QtQuick.Controls 2.4

import QtQuick.Window 2.2

import Qt.labs.settings 1.0


ApplicationWindow {
    id: control
    visible: true
    width: Screen.width/2
    height: Screen.height/2

    minimumWidth: Screen.width/4
    minimumHeight: Screen.height/4

    title: Qt.application.name
           + " ("+Qt.application.version+")"
           + (application.installed()?"":(" - "+qsTr("not installed").toUpperCase()))

    flags: Qt.Window
           //|Qt.CustomizeWindowHint
           |Qt.WindowTitleHint
           |Qt.WindowSystemMenuHint
           |Qt.WindowMinimizeButtonHint
           |Qt.WindowCloseButtonHint
           |Qt.WindowFullscreenButtonHint
           //|Qt.BypassGraphicsProxyWidget
           //|Qt.WindowMinMaxButtonsHint
           //|Qt.WindowMaximizeButtonHint

    property bool showBgImage: true
    background: Rectangle {
        border.width: 0
        color: "#080808"
        Loader {
            anchors.centerIn: parent
            active: control.showBgImage
            sourceComponent: Component {
                Image {
                    source: "qrc:///icons/uavos-logo.ico"
                    Text {
                        id: loadingText
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.bottom
                        color: "#888"
                        Connections {
                            target: application.appLog
                            onConsoleMessage: loadingText.text=msg
                        }
                    }
                }
            }
        }
    }


    visibility: Window.AutomaticVisibility

    Settings {
        id: settings
        category: "AppWindowState"
        property int x: Screen.width/4
        property int y: Screen.height/4
        property int width: Screen.width/2
        property int height: Screen.height/2
        property bool fullScreen: false
    }
    onVisibilityChanged: saveState()
    onXChanged: saveState()
    onYChanged: saveState()
    onWidthChanged: saveState()
    onHeightChanged: saveState()
    function saveState()
    {
        saveStateTimer.start()
    }
    Timer {
        id: saveStateTimer
        interval: 100
        onTriggered: {
            if(control.visibility==Window.Windowed
                    && control.width!==Screen.width
                    && control.height!==Screen.height){
                settings.x=control.x
                settings.y=control.y
                settings.width=control.width
                settings.height=control.height
            }
            settings.fullScreen=(visibility==Window.FullScreen)
        }
    }

    Component.onCompleted: {
        control.x=settings.x
        control.y=settings.y
        control.width=settings.width
        control.height=settings.height
        if(settings.fullScreen)control.showFullScreen()
    }

    onClosing: Qt.quit()

    //onActiveChanged:         console.log(active)

}
