/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Controls

import Apx.Menu

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
