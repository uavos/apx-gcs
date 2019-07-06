import QtQuick 2.2
import QtQml 2.12
import APX.Vehicles 1.0
import "../comm"
import "../pfd"
import "../common"

Rectangle {
    id: comm
    color: "black"
    property double itemWidth: height*5
    //anchors.fill: parent
    //anchors.rightMargin: parent.width*0.9
    anchors.topMargin: 3

    property bool isLanding:
        m.mode.value===mode_LANDING ||
        m.mode.value===mode_TAKEOFF ||
        m.mode.value===mode_TAXI ||
        (m.mode.value===mode_WPT && m.mtype.value===mtype_line)

    //warnings and errors bottom panel
    Text {
        id: warningsText
        anchors.fill: parent
        anchors.leftMargin: comm_row.width
        anchors.rightMargin: 1
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: height
        font.family: font_condenced
        Behavior on opacity { enabled: ui.smooth; PropertyAnimation {duration: 200} }
        Timer {
            id: fallTimer
            interval: 5000-200; running: false; repeat: false
            onTriggered: warningsText.opacity=0
        }
        Connections {
            target: apx.vehicles.current.warnings
            onShowMore: {
                switch(msgType){
                    default: warningsText.color="white"; break;
                    case VehicleWarnings.ERROR: warningsText.color="red"; break;
                    case VehicleWarnings.WARNING: warningsText.color="yellow"; break;
                }
                warningsText.text=msg;
                fallTimer.stop();
                warningsText.opacity=1;
                fallTimer.start();
            }
        }
    }


    Row {
        id: comm_row
        spacing: 1
        anchors.fill: parent
        CommNum {
            id: fuel_text
            property double v: m.fuel.value
            height: parent.height
            label: qsTr("FL")
            value: v.toFixed()
            toolTip: m.fuel.descr
            visible: v>0
            //width: itemWidth
            Rectangle{
                id: fuel_bar
                property double value: m.fuel.value
                property double vMax: 1
                property double v: m.fuel.value/vMax
                onValueChanged: vMax=value>vMax?value:vMax;
                anchors.right: parent.right
                anchors.rightMargin: 1
                //anchors.top: parent.verticalCenter
                anchors.bottom: parent.bottom
                //anchors.topMargin: 1
                anchors.bottomMargin: 1
                height: apx.limit(v*parent.height,1,parent.height)
                width: parent.width*0.05
                color: v<0.2?"red":v<0.3?"yellow":"green"
                ToolTipArea { text: m.fuel.descr }
                property int anim: (ui.smooth)?200:0
                Behavior on width { enabled: ui.smooth; PropertyAnimation {duration: fuel_bar.anim} }
                Behavior on color { enabled: ui.smooth; ColorAnimation {duration: fuel_bar.anim} }
            }
        }
        CommNum {
            id: dme_text
            property double v: m.dWPT.value
            height: parent.height
            label: qsTr("DME")
            value: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            toolTip: m.dWPT.descr
            width: itemWidth
        }
        CommNum {
            id: eta_text
            property int v: m.ETA.value
            property double valid: v>0
            property int tsec: ("0"+Math.floor(v%60)).slice(-2)
            property int tmin: ("0"+Math.floor(v/60)%60).slice(-2)
            property int thrs: Math.floor(v/60/60)
            property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
            height: parent.height
            label: qsTr("ETA")
            value: valid?sETA:"--:--"
            toolTip: m.ETA.descr
            width: itemWidth
        }
        CommNum {
            id: dh_text
            property double v: (m.mode.value===mode_TAXI)?m.delta.value:m.dHome.value
            height: parent.height
            label: qsTr("DH")
            value: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            width: itemWidth
            toolTip: m.dHome.descr
        }
        Wind {
            id: wind_arrow
            simplified: true
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            //height: scale_top.height*0.7
            width: height
            value: m.windHdg.value
        }
        Text {
            visible: wind_arrow.visible
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            text: m.windSpd.value.toFixed(1)
            font.pixelSize: parent.height
            font.family: font_narrow
            color: "white"
            ToolTipArea { text: m.windSpd.descr }
        }
        CommNum {
            id: wpt_text
            visible: ui.test || m.mode.value===mode_WPT
            height: parent.height
            label: qsTr("WPT")
            valueColor: "cyan"
            value: m.wpidx.value+1
            toolTip: m.wpidx.descr
        }

        CommNum {
            id: poi_text
            visible: ui.test || (m.mode.value===mode_STBY && m.loops.value>0)
            height: parent.height
            label: qsTr("LPS")
            valueColor: "cyan"
            value: m.loops.value
            toolTip: m.loops.descr
        }
        CommNum {
            id: rd_text
            visible: ui.test || isLanding
            property double v: m.rwDelta.value
            height: parent.height
            label: qsTr("RD")
            //value: m.rwDelta.value.toFixed(Math.abs(m.rwDelta.value)<10?1:0)+(m.rwAdj.value>0?"+"+m.rwAdj.value.toFixed():m.rwAdj.value<0?"-"+(-m.rwAdj.value).toFixed():"")
            value: (Math.abs(m.rwDelta.value)<1?0:m.rwDelta.value.toFixed())+(m.rwAdj.value>0?"+"+m.rwAdj.value.toFixed():m.rwAdj.value<0?"-"+(-m.rwAdj.value).toFixed():"")
            toolTip: m.rwDelta.descr
        }
        CommNum {
            id: turnR_text
            visible: ui.test || m.mode.value===mode_STBY || m.mode.value===mode_LANDING
            property double v: m.turnR.value
            height: parent.height
            label: qsTr("R")
            value: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            toolTip: m.turnR.descr
        }

    }

}
