import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Layouts 1.3

//import QtQuick 2.2
//import QtQuick.Controls 1.1
import "."
import "../components"

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
            value: mandala.current.recorder.recording
            toolTip: qsTr("Enable recording")
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: mandala.current.recorder.recording=!mandala.current.recorder.recording
            }
        }

        Text {
            property double v: mandala.current.recorder.size
            property string vSize: v>=(1024*1024)?(v/(1024*1024)).toFixed(1)+"MB":(v/1024).toFixed()+"KB"
            property string vTime: (Qt.formatTime(mandala.current.recorder.time,"hh")==="00"?"":Qt.formatTime(mandala.current.recorder.time,"hh")+":")+Qt.formatTime(mandala.current.recorder.time,"mm:ss")
            anchors.leftMargin: 2
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: vSize + "/"+ vTime
            font.pixelSize: parent.height   //*0.99
            font.family: font_narrow
            color: v>=(30*1024*1024)?"yellow":"gray"
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
        text: mandala.uavName
        font.pixelSize: parent.height   //*0.99
        font.family: font_narrow
        color: mandala.isLocal?"yellow":"#5f5"
        visible: mandala.test || mandala.dlcnt>0
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: menu.openSelectUAV() //menuUAV.open() //.popup(mouseX,mouseY)
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
            text: mandala.size+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            color: "yellow"
            visible: mandala.size>1
        }

        //ext servers
        CommBtn {
            id: btn_servers
            text: qsTr("EXT")
            color: "#5f5"
            value: datalink.extConnected
            inverted: true
            toolTip: qsTr("External servers")
            visible: datalink.serverNames.length>0
            width: height/0.5
            MouseArea {
                id: menuServersArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: menu.openServers()  //menuServers.open() //popup(mouseX,mouseY)
            }
        }

        //controls enable button
        CommBtn {
            id: btn_ctr
            text: qsTr("CTR")
            color: "#5f5"
            value: !mandala.readOnly
            inverted: true
            toolTip: qsTr("Enable controls")
            width: height/0.5
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: mandala.readOnly=!mandala.readOnly
            }
        }

        //comm stats
        CommNum {
            property string cRX: mandala.online?(mandala.dlinkData?"#8f8":"#aaf"):"red"
            property string cTX: mandala.readOnly?"red":"cyan"
            property string cER: mandala.errcnt>1?"yellow":"gray"
            height: parent.height
            label: qsTr("DL")
            toolTip: qsTr("Datalink statistics")
        }
        Text {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: "0%1".arg(mandala.dlcnt%100).slice(-2)+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            verticalAlignment: Text.AlignVCenter
            color: mandala.online?(mandala.dlinkData?"#8f8":"#aaf"):"red"
            ToolTipArea { text: qsTr("Received packets") }
        }
        Text {
            property int value: mandala.errcnt%1000
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: value+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            verticalAlignment: Text.AlignVCenter
            color: mandala.errcnt>1?(errTimer.running?"red":"yellow"):"gray"
            ToolTipArea { text: qsTr("Stream errors") }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                propagateComposedEvents: true
                onClicked: mandala.errcnt=0
            }
            Behavior on color { ColorAnimation {duration: mandala.smooth?250:0} }
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
            text: "0%1".arg(mandala.upcnt%100).slice(-2)+" "
            font.pixelSize: parent.height
            font.family: font_narrow
            verticalAlignment: Text.AlignVCenter
            color: mandala.readOnly?"red":"cyan"
            ToolTipArea { text: qsTr("Transmitted packets") }
        }


        CommNum {
            height: parent.height
            label: qsTr("VG")
            value: gcu_Ve.value.toFixed(1)
            valueColor: gcu_Ve.value>1?(gcu_Ve.value<7.4?"red":"yellow"):"gray"
            toolTip: gcu_Ve.descr
            //visible: mandala.test || gcu_Ve.value>1
        }


        CommNum {
            height: parent.height
            width: comm.itemWidth*1.5
            label: qsTr("RSS")
            ToolTipArea { text: RSS.descr + ", " + gcu_RSS.descr }
            Rectangle{
                id: rss_bar
                property double v: RSS.value
                anchors.right: parent.right
                anchors.rightMargin: 2
                anchors.top: parent.top
                anchors.bottom: parent.verticalCenter
                anchors.topMargin: 2
                anchors.bottomMargin: parent.height*0.1
                //height: parent.height*0.2
                width: mandala.limit(v*parent.width*0.6,1,parent.width*0.6)
                color: v<0.3?"red":v<0.7?"yellow":"green"
                ToolTipArea { text: RSS.descr }
                property int anim: (mandala.smooth)?200:0
                Behavior on width { PropertyAnimation {duration: rss_bar.anim} }
                Behavior on color { ColorAnimation {duration: rss_bar.anim} }
            }
            Rectangle{
                id: gcu_rss_bar
                property double v: gcu_RSS.value
                anchors.right: parent.right
                anchors.rightMargin: 2
                anchors.top: parent.verticalCenter
                //anchors.bottom: parent.bottom
                anchors.topMargin: 1
                //anchors.bottomMargin: parent.height*0.1+1
                height: rss_bar.height
                width: mandala.limit(v*parent.width*0.6,1,parent.width*0.6)
                color: v<0.3?"red":v<0.7?"yellow":"green"
                ToolTipArea { text: gcu_RSS.descr }
                property int anim: (mandala.smooth)?200:0
                Behavior on width { PropertyAnimation {duration: gcu_rss_bar.anim} }
                Behavior on color { ColorAnimation {duration: gcu_rss_bar.anim} }
            }
        }

    }

}
