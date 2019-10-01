import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

ListView {
    id: listView

    implicitHeight: 32

    property var modes: []

    orientation: ListView.Horizontal
    spacing: 8
    model: modes
    delegate: CleanButton {
        text: modelData
        toolTip: m.mode.descr+": "+modelData
        showText: true
        defaultHeight: listView.height
        ui_scale: 1
        titleSize: 0.5
        onTriggered: {
            m.mode.value=modelData
        }
        iconName: listView.modeIcon(modelData)
    }
    headerPositioning: ListView.OverlayHeader
    header: CleanButton {
        defaultHeight: listView.height
        ui_scale: 1
        text: m.mode.text
        property int v: m.mode.value
        property bool warning: v==mode_EMG || v==mode_RPV || v==mode_HOME || v==mode_TAXI
        property bool active: v==mode_LANDING || v==mode_TAKEOFF
        color: "#000"
        titleColor: warning?Material.color(Material.Yellow):active?Material.color(Material.Blue):Qt.darker(Material.primaryTextColor,1.5)
        onTriggered: {
            popupC.createObject(this)
        }
    }


    function modeIcon(mode)
    {
        switch(mode)
        {
        default: return ""
        case "EMG": return "google-controller"
        case "RPV": return "google-controller"
        case "UAV": return "directions-fork"
        case "WPT": return "navigation"
        case "STBY": return "clock-in"
        case "TAXI": return "taxi"
        case "TAKEOFF": return "airplane-takeoff"
        case "LANDING": return "airplane-landing"
        }
    }

    Component{
        id: popupC
        Popup {
            id: popup
            width: 150 //contentItem.implicitWidth
            height: contentItem.implicitHeight //Math.min(listView.implicitHeight, control.implicitHeight - topMargin - bottomMargin)
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
                model: m.mode.enumStrings
                highlightMoveDuration: 0
                delegate: ItemDelegate {
                    text: modelData
                    width: Math.max(listView.width,implicitWidth)
                    highlighted: text===m.mode.text
                    onClicked: {
                        popup.close()
                        m.mode.value=text
                    }
                }
                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }
}

