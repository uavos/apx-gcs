import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import Apx.Common 1.0
import Apx.Controls 1.0

import APX.Facts 1.0

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

    property int numberItemSize: Math.min(22,Math.max(12,height/15))
    property int overlayItemSize: numberItemSize

    property int margins: Math.max(1,numberItemSize*0.1)

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
        margins: 0
        showEditButton: false
        itemSize: numberItemSize
        //model.minimumWidth: 500
        color: "#00000000"
        settingsName: "video"
        defaults: [
            {"bind": "est.pos.hmsl", "title": "MSL", "prec": "0"},
            {"bind": "est.air.altitude", "title": "ALT", "prec": "0"},
        ]
    }

    Connections {
        enabled: !interactive
        target: application
        onLoadingFinished: {
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
        FactValue {
            id: timeItem
            Layout.fillHeight: true
            fact: f_time
            title: gps?"GPS":"LOCAL"
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
        FactValue {
            id: frameCntItem
            Layout.fillHeight: true
            showTitle: false
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
    FactValue {
        id: tposItem
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: timeLayout.bottom
        anchors.margins: control.margins
        height: overlayItemSize
        showTitle: false
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
    FactValue {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: control.margins
        height: overlayItemSize
        fact: f_gimbal_mode
        showTitle: false
        valueText: fact.text
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

        FactValue {
            Layout.fillHeight: true
            showTitle: false
            property var f: plugin.tune.controls
            value: qsTr("CTR")
            toolTip: f.descr
            visible: interactive
            enabled: true
            active: f.value
            onTriggered: f.value=!f.value

        }

        FactValue {
            Layout.fillHeight: true
            fact: f_zoom
            title: "zoom"
            valueText: (fact.value*100).toFixed()
            visible: value>0
            enabled: interactive
            onTriggered: fact.value=0
        }
        FactValue {
            Layout.fillHeight: true
            fact: f_focus
            title: "focus"
            valueText: (fact.value*100).toFixed()
            visible: value>0
            enabled: interactive
            onTriggered: fact.value=0
        }
        FactValue {
            Layout.fillHeight: true
            fact: f_ch
            title: "ch"
            visible: value>0
            onValueChanged: if(value>0)visible=true
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
            delegate: FactValue {
                Layout.fillHeight: true
                fact: modelData
                showTitle: false
                value: fact.value
                valueText: fact.name //.slice(fact.title.lastIndexOf("_")+1)
                visible: value
                onValueChanged: if(value)visible=true
                enabled: interactive
                onTriggered: fact.value=value?0:1
                active: value
            }
        }


    }

}
