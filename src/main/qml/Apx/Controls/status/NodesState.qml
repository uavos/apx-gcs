import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

Rectangle {
    border.width: 0
    color: "#000"
    implicitWidth: itemWidth
    Layout.margins: 1

    readonly property real maxItems: 17.5
    readonly property real aspectRatio: 5

    readonly property real itemWidth: height/maxItems*aspectRatio
    readonly property real itemHeight: itemWidth/aspectRatio

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        spacing: 0
        cacheBuffer: 0
        model: apx.vehicles.current.nodes.model
        snapMode: ListView.SnapToItem
        delegate: FactValue {
            implicitWidth: itemWidth
            implicitHeight: itemHeight
            fact: modelData
            descr: fact?fact.status+"\n"+fact.descr:""
            value: fact?fact.status:"" //.startsWith('[')?fact.size:""
            valueScale: 0.7
            valueColor: titleColor
            enabled: true
            onTriggered: {
                if(fact.active)apx.vehicles.current.nodes.request()
                else apx.vehicles.current.nodes.nstat()
            }
            titleColor: {
                if(fact){
                    if(fact.modified)return "#ffa" //Material.color(Material.Yellow)
                    if(fact.reconf)return "#faa" //Material.color(Material.Red)
                }
                return "#fff"
            }
        }
        headerPositioning: ListView.OverlayHeader
        header: FactValue {
            z: 100
            implicitWidth: itemWidth
            implicitHeight: itemHeight
            fact: apx.vehicles.current.nodes
            value: fact.status
            valueScale: 0.7
            enabled: true
            onTriggered: {
                if(fact.progress<0)fact.request()
                else fact.stop()
            }
        }
        ScrollBar.vertical: ScrollBar { width: 6 }
    }
}
