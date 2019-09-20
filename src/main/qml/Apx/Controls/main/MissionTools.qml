import QtQuick 2.5;
import QtQuick.Layouts 1.3
import QtPositioning 5.6

import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

import APX.Vehicles 1.0
import APX.Mission 1.0

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Menu 1.0
//import Apx.Map 1.0

RowLayout {
    property Mission mission: apx.vehicles.current.mission
    height: missionButton.height
    spacing: 10*ui.scale
    //property int itemSize: Math.max(10,missionButton.height)
    //property int iconFontSize: itemSize*0.8
    //property int titleFontSize: itemSize*0.8
    CleanButton {
        id: missionButton
        minimumWidth: height*3
        //implicitHeight: label.height+padding*2
        color: mission.modified?"#FFF59D":"#A5D6A7"
        Material.theme: Material.Light
        onMenuRequested: mission.requestMenu({"pos":Qt.point(0,ui.window.height)})
        onClicked: mission.trigger()
        onDoubleClicked: menuRequested()
        contents: [
            Label {
                id: label
                Layout.fillWidth: true
                Layout.fillHeight: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: missionButton.fontSize(missionButton.bodyHeight/2.3)
                text: (mission.status)
                      +"\n"+(mission.empty?"":mission.waypoints.descr)
            }
        ]
    }
    FactMenuAction {
        fact: mission.action.request
        visible: (!mission.synced)
    }
    FactMenuAction {
        fact: mission.action.upload
        visible: (!mission.synced) && (!mission.empty)
    }
    FactMenuAction {
        fact: mission.action.tools.action.save
        visible: (!mission.saved) && (!mission.empty)
    }
    FactMenuAction {
        fact: mission.action.tools.action.load
        visible: (mission.empty)
        onTriggered: fact.requestMenu({"pos":Qt.point(0,ui.window.height)})
    }
    FactMenuAction {
        fact: mission.missionTitle
        title: fact.descr
        visible: (!mission.empty) && mission.missionTitle.text===""
        onTriggered: mission.requestMenu({"pos":Qt.point(0,ui.window.height)})
    }
}
