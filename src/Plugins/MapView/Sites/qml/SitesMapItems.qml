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
import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtQml 2.12


MapItemGroup {
    id: places
    property bool showPlaces: apx.tools && sites
    property var map: ui.map
    property var sites: apx.tools.sites

    z: 1

    MapItemView {
        id: sitesView
        model: places.showPlaces?sites.lookup.model:0
        delegate: SiteItem { }
    }

    Component.onCompleted: {
        sites.lookup.area=Qt.binding(function(){return map.area})
        map.addMapItemView(sitesView)
    }


    //triggered site in lookup - focus on map
    Connections {
        target: sites.lookup
        function onItemTriggered(){
            map.showCoordinate(QtPositioning.coordinate(modelData.lat,modelData.lon))
        }
    }


    //CURRENT SITE LABEL
    Text {
        id: siteText
        parent: map //ui.main
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: map.height*0.15
        visible: text && ui.missionPlanner.showNavigation
        font.pixelSize: 24
        font.bold: true
        color: "#fff"
        text: apx.vehicles.current.mission.site
    }

}
