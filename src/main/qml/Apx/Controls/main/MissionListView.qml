import QtQuick 2.5;
import QtQuick.Layouts 1.3
import QtPositioning 5.6

import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

import APX.Vehicles 1.0
import APX.Mission 1.0

import Apx.Common 1.0
import Apx.Controls 1.0
//import Apx.Map 1.0

ColumnLayout {
    spacing: 4
    property Vehicle vehicle: apx.vehicles.current
    property Mission mission: vehicle.mission
    property bool empty: mission?mission.empty:true

    function focusOnMap(fact)
    {
        fact.trigger()
    }

    MissionTools {
        Layout.alignment: Qt.AlignTop
    }
    ListView {
        id: missionListView
        Layout.fillHeight: true
        model: mission.listModel
        implicitWidth: contentItem.childrenRect.width
        orientation: ListView.Vertical
        spacing: 4*ui.scale
        clip: true
        snapMode: ListView.SnapToItem
        visible: !empty


        delegate: RowLayout{
            spacing: 8*ui.scale
            CleanButton {
                font.family: font_mono
                font.bold: true
                defaultHeight: 16
                titleSize: 1

                titleColor: (fact && fact.modified)?Material.color(Material.Yellow):Material.primaryTextColor

                property var fact: modelData
                property string stitle: fact?fact.title:""
                property string sdescr: fact?fact.descr:""
                property string sstatus: fact?fact.status:""
                property int num: fact?fact.num:-1

                onClicked: fact.trigger()

                Component.onCompleted: {
                    if(!fact)return;
                    switch(fact.missionItemType){
                    case Mission.RunwayType:
                        color=Material.color(Material.Blue) //Style.cListRunway
                        title=Qt.binding(function(){return stitle})
                        iconItem.active=Qt.binding(function(){return vehicle.mandala.rwidx.value===num})
                        break;
                    case Mission.WaypointType:
                        color=Material.color(Material.BlueGrey) //Style.cListWaypoint
                        title=Qt.binding(function(){return stitle+(sdescr?" A":"")})
                        iconItem.active=Qt.binding(function(){return vehicle.mandala.wpidx.value===num})
                        break;
                    case Mission.PoiType:
                        color=Material.color(Material.Teal) //Style.cListPoint
                        title=Qt.binding(function(){return stitle+(sstatus?" "+sstatus:"")})
                        iconItem.active=Qt.binding(function(){return vehicle.mandala.piidx.value===num})
                        break;
                    case Mission.TaxiwayType:
                        color=Material.color(Material.Grey) //Style.cListTaxiway
                        title=Qt.binding(function(){return stitle})
                        iconItem.active=Qt.binding(function(){return vehicle.mandala.twidx.value===num})
                        break;
                    }

                }
            }
            Loader {
                id: iconItem
                active: false
                //asynchronous: true
                Layout.fillHeight: true
                sourceComponent: Component {
                    Label {
                        verticalAlignment: Text.AlignVCenter
                        font.family: "Material Design Icons"
                        font.pixelSize: Math.max(8,height)
                        text: materialIconChar["chevron-double-left"]
                        color: Material.iconColor
                    }
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
