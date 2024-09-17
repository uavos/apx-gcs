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
import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls

import Apx.Common

RowLayout {
    id: control
    readonly property int size: 20

    // mouse coordinate
    Item {
        id: site
        Layout.alignment: Qt.AlignVCenter

        implicitWidth: loaderSite.implicitWidth
        implicitHeight: loaderSite.implicitHeight
        
        Loader {
            id: loaderSite
            anchors.fill: parent
            // asynchronous: true
            property int viewIndex: 0
            sourceComponent: components[viewIndex%components.length]
            property var components: [ siteC, posC ]
            Component {
                id: posC
                Text {
                    font: apx.font_narrow(Style.fontSize)
                    color: "#fff"
                    property var c: map.mouseCoordinate
                    text: apx.latToString(c.latitude)+" "+apx.lonToString(c.longitude)
                }
            }
            Component {
                id: siteC
                Text {
                    font: apx.font_narrow(Style.fontSize)
                    color: "#fff"
                    text: apx.vehicles.current.mission.site
                }
            }
        }
        ToolTipArea {
            cursorShape: Qt.PointingHandCursor
            onClicked: loaderSite.viewIndex=loaderSite.viewIndex+1
            text: qsTr("Switch views")
        }
    }

    //tiles downloader
    BusyIndicator {
        id: busy

        Layout.alignment: Qt.AlignVCenter

        property var fact: apx.tools?apx.tools.location:null
        property string text: fact?fact.text:""
        property int progress: fact?fact.progress:-1

        padding: 1
        implicitHeight: control.size
        implicitWidth: implicitHeight

        running: progress>=0
        Text {
            anchors.fill: parent
            color: "#60FFFFFF"
            text: busy.text
            font: apx.font_narrow(height*0.6)
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                busy.fact.abort();
            }
        }
    }

    ValueButton {
        Layout.alignment: Qt.AlignVCenter

        size: control.size
        fact: (apx.tools && apx.tools.location)?apx.tools.location.offline:null
        showText: false
        showIcon: false
        value: (active?qsTr("offline"):qsTr("online")).toUpperCase()
        active: fact && fact.value
        enabled: true
        onTriggered: fact.value=!fact.value
        visible: active
        onActiveChanged: visible=true
    }

    // map scale and distance measure
    Item {
        id: scale
        Layout.alignment: Qt.AlignVCenter

        implicitWidth: loaderScale.implicitWidth
        implicitHeight: loaderScale.implicitHeight
        
        Loader {
            id: loaderScale
            // asynchronous: true
            property int viewIndex: 0
            sourceComponent: components[viewIndex%components.length]
            property var components: [ scaleC, distC ]

            Component {
                id: scaleC
                MapScale { width: Style.buttonSize*3 }
            }

            Component {
                id: distC
                MapDistance { width: Style.buttonSize*3 }
            }
        }
        ToolTipArea {
            cursorShape: Qt.PointingHandCursor
            onClicked: loaderScale.viewIndex=loaderScale.viewIndex+1
            text: qsTr("Switch views")
        }
    }

    // travel path
    Item {
        id: pathItem
        Layout.alignment: Qt.AlignVCenter

        implicitHeight: control.size
        implicitWidth: Math.max(icon.width+textItem.implicitWidth, height*4)

        opacity: apx.vehicles.current.totalDistance>0?1:0.5
        property string text: apx.distanceToString(apx.vehicles.current.totalDistance)
        MaterialIcon {
            id: icon
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            name: "airplane"
            rotation: 90
            size: height
        }
        Text {
            id: textItem
            anchors.left: icon.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            verticalAlignment: Text.AlignVCenter
            font: apx.font_narrow(Style.fontSize)
            color: "#fff"
            text: pathItem.text
        }
        ToolTipArea {
            text: qsTr("Distance travelled")
            cursorShape: Qt.PointingHandCursor
            onClicked: apx.vehicles.current.telemetry.rpath.trigger()
        }
    }

}
