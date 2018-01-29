import QtQuick 2.5;
import QtQuick.Layouts 1.3
import QtPositioning 5.6

import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

import GCS.FactSystem 1.0
import GCS.Vehicles 1.0
import GCS.Mission 1.0
import "."
import ".."

ColumnLayout {
    spacing: 4
    implicitWidth: missionListView.width
    property var mission: app.vehicles.current.mission
    property bool empty: mission?mission.empty:true

    onEmptyChanged: if(!empty)showMissionOnMap()

    function focusOnMap(fact)
    {
        //map.centerOnCoordinate(QtPositioning.coordinate(fact.latitude.value,fact.longitude.value))
        fact.trigger()
    }
    function showMissionOnMap()
    {
        map.tilt=0
        map.bearing=0
        map.visibleRegion=mission.boundingGeoRectangle()
    }

    RowLayout {
        Layout.alignment: Qt.AlignTop
        height: missionButton.height
        spacing: 10
        MapButton {
            id: missionButton
            minWidth: height*3
            text: mission.title+"\n"+(empty?qsTr("push to request"):mission.waypoints.descr)
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Qt.application.font.pixelSize * 0.8
            color: mission.modified?"#FFFF00":"#FFFFFF"
            textColor: "black"
            defaultOpacity: 1
            onMenuRequested: map.showFactMenu(mission)
            onClicked: {
                if(empty){
                    mission.request.trigger()
                }else{
                    showMissionOnMap()
                }
            }
        }
        MapButton {
            id: uploadButton
            text: qsTr("Upload")
            horizontalAlignment: Text.AlignHCenter
            minWidth: height*3
            minHeight: parent.height
            border.width: 1
            border.color: Style.cGreen
            color: "#2a4"
            textColor: "white"
            defaultOpacity: 0.8
            onClicked: mission.upload.trigger()
            //x: visible?missionButton.width+10:0
            visible: mission.modified
            //Behavior on x { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
        }
    }
    ListView {
        id: missionListView
        Layout.fillHeight: true
        model: mission.listModel
        implicitWidth: contentItem.childrenRect.width
        orientation: ListView.Vertical
        spacing: 4
        clip: true
        snapMode: ListView.SnapToItem
        visible: !empty


        delegate: MapButton {
            id: btn
            text: (hovered && descr)?label+"\n"+descr:label
            font.family: font_mono
            font.bold: true
            defaultOpacity: 0.9
            font.pixelSize: Qt.application.font.pixelSize * map.itemsScaleFactor

            border.width: 1
            border.color: "#80FFFFFF"

            textColor: (fact && fact.modified)?"#FFFF00":"#FFFFFF"

            property var fact: modelData
            property string title: fact?fact.title:""
            property string label //: title
            property string descr //: modelData.status
            property string status: fact?fact.status:""

            onMenuRequested: {
                fact.trigger()
                map.showFactMenu(fact)
            }
            onClicked: fact.trigger()

            Component.onCompleted: {
                switch(fact.missionItemType){
                case Mission.RunwayType:
                    color=Style.cListRunway
                    label=Qt.binding(function(){return title})
                    break;
                case Mission.WaypointType:
                    color=Style.cListWaypoint
                    label=Qt.binding(function(){return title.split(' ').slice(0,3).join(' ')+(descr?" A":"")})
                    descr=Qt.binding(function(){return title.split(' ').slice(3).join(' ')})
                    break;
                case Mission.PoiType:
                    color=Style.cListPoint
                    label=Qt.binding(function(){return title.split(' ').slice(0,2).join(' ')})
                    descr=Qt.binding(function(){return title.split(' ').slice(2).join(' ')})
                    break;
                case Mission.TaxiwayType:
                    color=Style.cListTaxiway
                    label=Qt.binding(function(){return title})
                    descr=Qt.binding(function(){return status})
                    break;
                }

            }
        }


        //! [transitions]
        remove: Transition {
            SequentialAnimation {
                PauseAnimation { duration: 125 }
                NumberAnimation { property: "height"; to: 0; easing.type: Easing.InOutQuad }
            }
        }

        displaced: Transition {
            SequentialAnimation {
                PauseAnimation { duration: 125 }
                NumberAnimation { property: "y"; easing.type: Easing.InOutQuad }
            }
        }
        //! [transitions]
    }
}
