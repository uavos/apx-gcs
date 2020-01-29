import QtQuick 2.3
import QtQuick.Layouts 1.3

import Apx.Common 1.0

FactValue {
    id: rssControl
    title: qsTr("RSS")

    readonly property var f_rss: mandala.sns.com.rss
    readonly property real m_rss: f_rss.value

    readonly property var f_gcs_rss: apx.vehicles.local.mandalatree.sns.com.rss
    readonly property real m_gcs_rss: f_gcs_rss.value


    descr: f_rss.descr+ "\n" + qsTr("Onboard") + "/" + qsTr("Ground")

    property real value1: Math.min(1,m_rss)
    property real value2: Math.min(1,m_gcs_rss)

    property color barColorBG: "#555"

    property int barHeight: rssControl.height/2*0.5
    property int barWidth: rssControl.width*0.6

    function barColor(v)
    {
        return v<0.3?"red":v<0.7?"yellow":"green"
    }

    enabled: true
    onClicked: apx.datalink.trigger()

    contents: [
        ColumnLayout {
            //anchors.fill: parent
            spacing: rssControl.height*0.1
            //Layout.fillHeight: true
            //Layout.fillWidth: true
            Rectangle {
                implicitHeight: barHeight
                implicitWidth: barWidth
                border.width: 0
                color: barColorBG
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: Math.min(barWidth,barWidth-barWidth*value1)
                    border.width: 0
                    color: barColor(value1)
                }
            }
            Rectangle {
                implicitHeight: barHeight
                implicitWidth: barWidth
                border.width: 0
                color: barColorBG
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: Math.min(barWidth,barWidth-barWidth*value2)
                    border.width: 0
                    color: barColor(value2)
                }
            }
        }
    ]
}
