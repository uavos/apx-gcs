import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Layouts 1.3
import APX.Vehicles 1.0

//import QtQuick 2.2
//import QtQuick.Controls 1.1
import "."
import "../common"

Rectangle {
    id: comm
    color: "black"
    property double itemWidth: height*3
    //anchors.fill: parent
    //anchors.rightMargin: parent.width*0.9
    //Material.theme: Material.Dark
    //Material.primary: "#41cd52"
    //Material.accent: "#41cd52"
    //Material.background: Material.Teal

    Row {
        id: rec_row
        spacing: 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        CommBtn {
            id: btn_rec
            text: qsTr("REC")
            color: "#5f5"
            value: apx.vehicles.current.telemetry.active
            toolTip: qsTr("Enable recording")
            MouseArea {
                enabled: apx.vehicles.current.telemetry.recorder.value?true:false
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: apx.vehicles.current.telemetry.recorder.value=!apx.vehicles.current.telemetry.recorder.value
            }
        }

        Text {
            anchors.leftMargin: 2
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: apx.vehicles.current.telemetry.status
            font.pixelSize: parent.height   //*0.99
            font.family: font_narrow
            color: apx.vehicles.current.telemetry.active?"#5f5":"gray"
        }
    }

    Text {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: rec_row.right
        anchors.right: comm_row.left
        anchors.topMargin: 1
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        text: apx.vehicles.current.callsign
        font.pixelSize: parent.height   //*0.99
        font.family: font_narrow
        color: apx.vehicles.current.vehicleClass===Vehicle.LOCAL?"yellow":"#5f5"
        visible: ui.test || apx.datalink.valid
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: apx.vehicles.select.trigger() //menuUAV.open() //.popup(mouseX,mouseY)
        }
    }

    Row {
        id: comm_row
        spacing: 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        Text {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            verticalAlignment: Text.AlignVCenter
            text: apx.vehicles.list.size+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            color: "yellow"
            visible: apx.vehicles.list.size>0
        }

        //ext servers
        CommBtn {
            id: btn_servers
            text: qsTr("EXT")
            color: "#5f5"
            value: apx.datalink.hosts.connectedCount>0
            inverted: true
            toolTip: qsTr("External servers")
            visible: ui.test || apx.datalink.hosts.availableCount>0
            width: height/0.5
            MouseArea {
                id: menuServersArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: apx.datalink.servers.trigger()
            }
        }

        //controls enable button
        CommBtn {
            id: btn_ctr
            text: qsTr("CTR")
            color: "#5f5"
            value: !apx.datalink.readonly.value
            inverted: true
            toolTip: qsTr("Enable controls")
            width: height/0.5
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: apx.datalink.readonly.value=!apx.datalink.readonly.value
            }
        }

        //comm stats
        CommNum {
            //property string cRX: apx.datalink.online?(apx.vehicles.current.streamType===Vehicle.TELEMETRY?"#8f8":"#aaf"):"red"
            //property string cTX: apx.datalink.readonly.value?"red":"cyan"
            //property string cER: m.errcnt>1?"yellow":"gray"
            height: parent.height
            label: qsTr("DL")
            toolTip: qsTr("Comm statistics")
        }
        Text {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: "0%1".arg(apx.datalink.stats.dnlink.cnt.value%100).slice(-2)+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            verticalAlignment: Text.AlignVCenter
            color: apx.datalink.online?(apx.vehicles.current.streamType===Vehicle.TELEMETRY?"#8f8":"#aaf"):"red"
            ToolTipArea { text: qsTr("Received packets") }
        }
        Text {
            property int value: m.errcnt%1000
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: value+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            verticalAlignment: Text.AlignVCenter
            color: m.errcnt>1?(errTimer.running?"red":"yellow"):"gray"
            ToolTipArea { text: qsTr("Stream errors") }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                propagateComposedEvents: true
                onClicked: m.errcnt=0
            }
            Behavior on color { enabled: ui.smooth; ColorAnimation {duration: 250} }
            Timer {
                id: errTimer
                interval: 5000
                repeat: false
            }
            onValueChanged: errTimer.restart()
        }
        Text {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: "0%1".arg(apx.datalink.stats.uplink.cnt.value % 100).slice(-2)+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            verticalAlignment: Text.AlignVCenter
            color: apx.datalink.readonly.value?"red":"cyan"
            ToolTipArea { text: qsTr("Transmitted packets") }
        }


        CommNum {
            height: parent.height
            label: qsTr("VG")
            value: m.gcu_Ve.value.toFixed(1)
            valueColor: m.gcu_Ve.value>1?(m.gcu_Ve.value<7.4?"red":"yellow"):"gray"
            toolTip: m.gcu_Ve.descr
            //visible: ui.test || m.gcu_Ve.value>1
        }


        CommNum {
            height: parent.height
            width: comm.itemWidth*1.5
            label: qsTr("RSS")
            ToolTipArea { text: m.RSS.descr + ", " + m.gcu_RSS.descr }
            Rectangle{
                id: rss_bar
                property double v: m.RSS.value
                anchors.right: parent.right
                anchors.rightMargin: 2
                anchors.top: parent.top
                anchors.bottom: parent.verticalCenter
                anchors.topMargin: 2
                anchors.bottomMargin: parent.height*0.1
                //height: parent.height*0.2
                width: apx.limit(v*parent.width*0.6,1,parent.width*0.6)
                color: v<0.3?"red":v<0.7?"yellow":"green"
                ToolTipArea { text: m.RSS.descr }
                property int anim: (ui.smooth)?200:0
                Behavior on width { enabled: ui.smooth; PropertyAnimation {duration: rss_bar.anim} }
                Behavior on color { enabled: ui.smooth; ColorAnimation {duration: rss_bar.anim} }
            }
            Rectangle{
                id: gcu_rss_bar
                property double v: m.gcu_RSS.value
                anchors.right: parent.right
                anchors.rightMargin: 2
                anchors.top: parent.verticalCenter
                //anchors.bottom: parent.bottom
                anchors.topMargin: 1
                //anchors.bottomMargin: parent.height*0.1+1
                height: rss_bar.height
                width: apx.limit(v*parent.width*0.6,1,parent.width*0.6)
                color: v<0.3?"red":v<0.7?"yellow":"green"
                ToolTipArea { text: m.gcu_RSS.descr }
                property int anim: (ui.smooth)?200:0
                Behavior on width { enabled: ui.smooth; PropertyAnimation {duration: gcu_rss_bar.anim} }
                Behavior on color { enabled: ui.smooth; ColorAnimation {duration: gcu_rss_bar.anim} }
            }
        }

    }

}
