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
import QtQuick.Layouts

import Apx.Common
import Apx.Controls

import APX.Facts

Item {
    id: control

    readonly property var f_time: mandala.est.sys.time
    readonly property var f_gimbal_mode: mandala.cmd.gimbal.mode
    readonly property var f_zoom: mandala.cmd.cam.zoom
    readonly property var f_focus: mandala.cmd.cam.focus
    readonly property var f_ch: mandala.cmd.cam.ch
    readonly property var f_PF: mandala.cmd.cam.pf
    readonly property var f_NIR: mandala.cmd.cam.nir
    readonly property var f_FM: mandala.cmd.cam.fm
    readonly property var f_FT: mandala.cmd.cam.ft
    readonly property var f_RNG: mandala.cmd.cam.range

    readonly property real m_lat: mandala.est.cam.lat.value
    readonly property real m_lon: mandala.est.cam.lon.value
    readonly property real m_hmsl: mandala.est.cam.hmsl.value

    property bool interactive: false
    property bool alive: true

    property real numberItemSize: Math.min(22,Math.max(12,height/15))
    property real overlayItemSize: numberItemSize

    property real margins: Math.max(1,numberItemSize*0.1)

    Component.onCompleted: {
        if(!interactive)return
        for(var i in overlays){
            var overlay=overlays[i]
            var c=numbersMenuC.createObject(control,{"overlay": overlay})
            c.parentFact=plugin.tune.overlay
            plugin.overlayNumbersChanged.connect(overlay.model.loadSettings)
        }
    }
    Component {
        id: numbersMenuC
        NumbersMenu {
            property var overlay
            defaults: overlay.defaults
            settingsName: overlay.settingsName
            destroyOnClose: false
            onAccepted: plugin.overlayNumbersChanged()
        }
    }

    property var overlays: [overlay_left]

    /*NumbersBar {
        id: overlay_bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: control.margins
        showEditButton: false
        itemSize: numberItemSize
        settingsName: "video_bottom"
        defaults: [
            //{"bind": "altitude", "title": "ALT", "prec": "0"},
        ]
    }*/

    NumbersBox {
        id: overlay_left
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: control.margins
        showEditButton: false
        itemSize: numberItemSize
        //model.minimumWidth: 500
        color: "#00000000"
        settingsName: "video"
        defaults: [
            {"bind": "est.pos.hmsl", "title": "MSL", "prec": "0"},
            {"bind": "est.pos.altitude", "title": "ALT", "prec": "0"},
        ]
    }

    Connections {
        enabled: !interactive
        target: application
        function onLoadingFinished()
        {
            //plugin is available
            for(var i in overlays){
                var overlay=overlays[i]
                plugin.overlayNumbersChanged.connect(overlay.model.loadSettings)
            }
        }
    }


    RowLayout {
        id: timeLayout
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.margins: control.margins
        spacing: overlayItemSize/2
        height: overlayItemSize
        ValueButton {
            id: timeItem
            Layout.fillHeight: true
            fact: f_time
            text: gps?"GPS":"LOCAL"
            value: apx.dateToString(time)
            property bool gps: false
            property int time: gps?gpsTime:localTime

            property int gpsTime: fact.value
            property int localTime: 0
            Timer {
                running: !timeItem.gps
                interval: 500
                repeat: true
                onTriggered: {
                    var date = new Date;
                    timeItem.localTime=date.getTime()/1000
                }
            }
            onGpsTimeChanged: {
                gpsTimeout.restart()
                timeItem.gps=gpsTime>0
            }
            Timer {
                id: gpsTimeout
                interval: 5000
                onTriggered: timeItem.gps=false
            }
            enabled: interactive
            onTriggered: plugin.tune.tools.trigger()
        }

        //frame cnt
        ValueButton {
            id: frameCntItem
            Layout.fillHeight: true
            showText: false
            readonly property int v: plugin.frameCnt
            value: ("0"+v).slice(-2)
            visible: alive && v>0
            onValueChanged: frameTimeout.restart()
            warning: visible && !frameTimeout.running
            Timer {
                id: frameTimeout
                interval: 1000
                repeat: false
            }
        }
    }

    //cam track pos
    ValueButton {
        id: tposItem
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: timeLayout.bottom
        anchors.margins: control.margins
        height: overlayItemSize
        showText: false
        property real lat: m_lat
        property real lon: m_lon
        property real hmsl: m_hmsl
        visible: lat!=0 && lon!=0 && (tposTimeout.running||active)
        value: apx.latToString(lat)+" "+apx.lonToString(lon)+(hmsl!=0?" "+apx.distanceToString(hmsl, false):"")

        onLatChanged: tposTimeout.restart()
        onLonChanged: tposTimeout.restart()
        onHmslChanged: tposTimeout.restart()
        Timer {
            id: tposTimeout
            interval: 10000
            repeat: false
        }
        enabled: interactive
        onTriggered: active=!active
    }



    //cam mode
    ValueButton {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: control.margins
        height: overlayItemSize
        fact: f_gimbal_mode
        showText: false
        enabled: interactive
        onTriggered: popupC.createObject(this)
        Component{
            id: popupC
            Popup {
                id: popup
                width: 150
                height: contentItem.implicitHeight
                topMargin: 6
                bottomMargin: 6
                padding: 0
                margins: 0
                x: parent.width

                Component.onCompleted: open()
                onClosed: destroy()

                contentItem: ListView {
                    id: listView
                    implicitHeight: contentHeight
                    implicitWidth: contentWidth
                    model: f_gimbal_mode.enumStrings
                    highlightMoveDuration: 0
                    delegate: ItemDelegate {
                        text: modelData
                        width: Math.max(listView.width,implicitWidth)
                        highlighted: text===f_gimbal_mode.text
                        onClicked: {
                            popup.close()
                            f_gimbal_mode.value=text
                        }
                    }
                    ScrollIndicator.vertical: ScrollIndicator { }
                }
            }
        }
    }

    //bottom cam opts and values
    RowLayout {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        //anchors.right: parent.right
        anchors.margins: control.margins
        height: overlayItemSize
        spacing: 3

        ValueButton {
            Layout.fillHeight: true
            showText: false
            property var f: plugin.tune.controls
            value: qsTr("CTR")
            toolTip: f.descr
            visible: interactive
            enabled: true
            active: f.value
            onTriggered: f.value=!f.value

        }

        ValueButton {
            Layout.fillHeight: true
            fact: f_zoom
            text: "zoom"
            value: (fact.value*100).toFixed()
            visible: fact.value>0
            enabled: interactive
            onTriggered: fact.value=0
        }
        ValueButton {
            Layout.fillHeight: true
            fact: f_focus
            text: "focus"
            value: (fact.value*100).toFixed()
            visible: fact.value>0
            enabled: interactive
            onTriggered: fact.value=0
        }
        ValueButton {
            Layout.fillHeight: true
            fact: f_ch
            text: "ch"
            visible: fact.value>0
            onValueChanged: if(fact.value>0)visible=true
            enabled: interactive
            onTriggered: fact.value=0
        }

        Repeater {
            model: [
                f_PF,
                f_NIR,
                f_FM,
                f_FT,
                f_RNG,
            ]
            delegate: ValueButton {
                Layout.fillHeight: true
                fact: modelData
                showText: false
                value: fact.name //.slice(fact.title.lastIndexOf("_")+1)
                visible: fact.value
                onValueChanged: if(fact.value)visible=true
                enabled: interactive
                onTriggered: fact.value=fact.value?0:1
                active: fact.value
            }
        }


    }

}
