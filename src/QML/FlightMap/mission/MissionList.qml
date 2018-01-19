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
    width: missionList.width
    visible: !app.vehicles.current.mission.empty

    function focusOnMap(fact)
    {
        //map.centerOnCoordinate(QtPositioning.coordinate(fact.latitude.value,fact.longitude.value))
        fact.trigger()
    }

    RowLayout {
        height: missionButton.height
        spacing: 10
        MapButton {
            id: missionButton
            property var fact: app.vehicles.current.mission
            minWidth: height*3
            text: fact.title+"\n"+fact.waypoints.descr
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Qt.application.font.pixelSize * 0.8
            color: fact.modified?"#FFFF00":"#FFFFFF"
            textColor: "black"
            defaultOpacity: 1
            onMenuRequested: map.showFactMenu(fact)
            onClicked: {
                map.tilt=0
                map.bearing=0
                map.visibleRegion=fact.boundingGeoRectangle()
            }
        }
        MapButton {
            id: uploadButton
            //z: missionButton.z-1
            property var fact: app.vehicles.current.mission
            text: qsTr("Upload")
            horizontalAlignment: Text.AlignHCenter
            minWidth: height*3
            minHeight: parent.height
            border.width: 1
            border.color: Style.cGreen
            color: "#3f3"
            textColor: "white"
            defaultOpacity: 0.8
            onClicked: fact.upload.trigger()
            //x: visible?missionButton.width+10:0
            visible: fact.modified
            //Behavior on x { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
        }
    }
    ListView {
        id: missionList
        Layout.fillHeight: true
        model: app.vehicles.current.mission.listModel
        implicitWidth: contentItem.childrenRect.width
        orientation: ListView.Vertical
        spacing: 4
        clip: true
        snapMode: ListView.SnapToItem

        delegate: MapButton {
            id: btn
            text: (hovered && descr)?label+"\n"+descr:label
            font.family: font_mono
            font.bold: true
            defaultOpacity: 0.9
            font.pixelSize: Qt.application.font.pixelSize * map.itemsScaleFactor

            border.width: 1
            border.color: "#80FFFFFF"

            textColor: modelData.modified?"#FFFF00":"#FFFFFF"

            property Fact fact: modelData
            property string title: fact?fact.title:""
            property string label //: title
            property string descr //: modelData.status

            onMenuRequested: {
                modelData.trigger()
                map.showFactMenu(modelData)
            }
            onClicked: modelData.trigger()

            Component.onCompleted: {
                switch(modelData.missionItemType){
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
                    descr=Qt.binding(function(){return modelData.status})
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
