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
    //spacing: 10*ui.scale
    //property int itemSize: Math.max(10,missionButton.height)
    //property int iconFontSize: itemSize*0.8
    //property int titleFontSize: itemSize*0.8
    TextButton {
        id: missionButton
        minimumWidth: height*3
        color: mission.modified?"#FFF59D":"#A5D6A7"
        Material.theme: Material.Light
        onClicked: mission.trigger()
        text: (mission.text)
              +"\n"+(mission.empty?"":mission.waypoints.descr)
        textScale: 0.45
    }
    ActionButton {
        fact: mission.request
        showText: false
        visible: (!mission.synced)
    }
    ActionButton {
        fact: mission.upload
        showText: false
        visible: (!mission.synced) && (!mission.empty)
    }
    ActionButton {
        fact: mission.tools.save
        showText: false
        visible: (!mission.saved) && (!mission.empty)
    }
    ActionButton {
        fact: mission.tools.load
        showText: false
        visible: (mission.empty)
    }
}
