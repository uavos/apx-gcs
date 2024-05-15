/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Layouts
import QtPositioning

import QtQuick.Controls
import QtQuick.Controls.Material

import APX.Vehicles as APX
import APX.Mission

import Apx.Common
import Apx.Controls

ColumnLayout {
    id: control
    spacing: Style.spacing*2
    
    readonly property APX.Vehicle vehicle: apx.vehicles.current
    
    readonly property Mission mission: vehicle.mission
    readonly property bool empty: mission?mission.empty:true

    function focusOnMap(fact)
    {
        fact.trigger()
    }

    ListView {
        id: missionListView
        Layout.fillHeight: true
        model: mission.listModel
        implicitWidth: contentItem.childrenRect.width
        orientation: ListView.Vertical
        spacing: Style.spacing/2
        clip: true
        snapMode: ListView.SnapToItem
        visible: !empty


        delegate: RowLayout{
            spacing: Style.spacing
            TextButton {
                size: Style.buttonSize*0.7

                toolTip: modelData?modelData.descr:""

                text: {
                    if(!modelData)return ""
                    var a=[]
                    a.push(modelData.title)
                    if(modelData.descr)
                        a.push('>')                    
                    return a.join(' ')
                }

                onTriggered: {
                    if(modelData.selected)modelData.trigger()
                    else modelData.selected=true
                }

                textColor: modelData.modified
                    ? Material.color(Material.Yellow)
                    : Material.primaryTextColor


                color: {
                    switch(modelData.missionItemType){
                    default: return Material.primaryColor
                    case Mission.RunwayType:
                        return Material.color(Material.Blue, Material.Shade900) //Style.cListRunway
                    case Mission.WaypointType:
                        return Material.color(Material.BlueGrey, Material.Shade500) //Style.cListWaypoint
                    case Mission.PoiType:
                        return Material.color(Material.Teal, Material.Shade700) //Style.cListPoint
                    case Mission.TaxiwayType:
                        return Material.color(Material.Grey, Material.Shade800) //Style.cListTaxiway
                    }
                }
            }
            Loader {
                active: modelData.active
                Layout.fillHeight: true
                sourceComponent: Component {
                    MaterialIcon {
                        verticalAlignment: Text.AlignVCenter
                        size: Math.max(8,height)
                        name: "chevron-double-left"
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
