import QtQuick 2.2
//import QtQuick.Controls 1.1
import "../pfd"
import "../comm"
import "../components"

Rectangle {
    id: comm
    color: "black"
    property double itemWidth: height*5
    //anchors.fill: parent
    //anchors.rightMargin: parent.width*0.9
    anchors.topMargin: 3

    property bool isLanding:
        mode.value===mode_LANDING ||
        mode.value===mode_TAKEOFF ||
        mode.value===mode_TAXI ||
        (mode.value===mode_WPT && mtype.value===mtype_line)

    TextLine {
        id: warnings
        anchors.fill: parent
        anchors.leftMargin: comm_row.width
        Connections {
            target: mandala.current
            onWarningChanged: warnings.show(mandala.current.warning,"yellow")
            onAlarmChanged: warnings.show(mandala.current.alarm,"red")
        }
    }

    Row {
        id: comm_row
        spacing: 1
        anchors.fill: parent
        CommNum {
            id: fuel_text
            property double v: fuel.value
            height: parent.height
            label: qsTr("FL")
            value: v.toFixed()
            toolTip: fuel.descr
            visible: v>0
            //width: itemWidth
            Rectangle{
                id: fuel_bar
                property double value: fuel.value
                property double vMax: 1
                property double v: fuel.value/vMax
                onValueChanged: vMax=value>vMax?value:vMax;
                anchors.right: parent.right
                anchors.rightMargin: 1
                //anchors.top: parent.verticalCenter
                anchors.bottom: parent.bottom
                //anchors.topMargin: 1
                anchors.bottomMargin: 1
                height: sys.limit(v*parent.height,1,parent.height)
                width: parent.width*0.05
                color: v<0.2?"red":v<0.3?"yellow":"green"
                ToolTipArea { text: fuel.descr }
                property int anim: (app.settings.smooth.value)?200:0
                Behavior on width { PropertyAnimation {duration: fuel_bar.anim} }
                Behavior on color { ColorAnimation {duration: fuel_bar.anim} }
            }
        }
        CommNum {
            id: dme_text
            property double v: dWPT.value
            height: parent.height
            label: qsTr("DME")
            value: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            toolTip: dWPT.descr
            width: itemWidth
        }
        CommNum {
            id: eta_text
            property int v: ETA.value
            property double valid: v>0
            property int tsec: ("0"+Math.floor(v%60)).slice(-2)
            property int tmin: ("0"+Math.floor(v/60)%60).slice(-2)
            property int thrs: Math.floor(v/60/60)
            property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
            height: parent.height
            label: qsTr("ETA")
            value: valid?sETA:"--:--"
            toolTip: ETA.descr
            width: itemWidth
        }
        CommNum {
            id: dh_text
            property double v: (mode.value===mode_TAXI)?delta.value:dHome.value
            height: parent.height
            label: qsTr("DH")
            value: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            width: itemWidth
            toolTip: dHome.descr
        }
        Wind {
            id: wind_arrow
            simplified: true
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            //height: scale_top.height*0.7
            width: height
            value: windHdg.value
        }
        Text {
            visible: wind_arrow.visible
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            text: windSpd.value.toFixed(1)
            font.pixelSize: parent.height
            font.family: font_narrow
            color: "white"
            ToolTipArea { text: windSpd.descr }
        }
        CommNum {
            id: wpt_text
            visible: app.settings.test.value || mode.value===mode_WPT
            height: parent.height
            label: qsTr("WPT")
            valueColor: "cyan"
            value: wpidx.value+1
            toolTip: wpidx.descr
        }

        CommNum {
            id: poi_text
            visible: app.settings.test.value || (mode.value===mode_STBY && loops.value>0)
            height: parent.height
            label: qsTr("LPS")
            valueColor: "cyan"
            value: loops.value
            toolTip: loops.descr
        }
        CommNum {
            id: rd_text
            visible: app.settings.test.value || isLanding
            property double v: rwDelta.value
            height: parent.height
            label: qsTr("RD")
            //value: rwDelta.value.toFixed(Math.abs(rwDelta.value)<10?1:0)+(rwAdj.value>0?"+"+rwAdj.value.toFixed():rwAdj.value<0?"-"+(-rwAdj.value).toFixed():"")
            value: (Math.abs(rwDelta.value)<1?0:rwDelta.value.toFixed())+(rwAdj.value>0?"+"+rwAdj.value.toFixed():rwAdj.value<0?"-"+(-rwAdj.value).toFixed():"")
            toolTip: rwDelta.descr
        }
        CommNum {
            id: turnR_text
            visible: app.settings.test.value || mode.value===mode_STBY || mode.value===mode_LANDING
            property double v: turnR.value
            height: parent.height
            label: qsTr("R")
            value: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            toolTip: turnR.descr
        }

    }

}
