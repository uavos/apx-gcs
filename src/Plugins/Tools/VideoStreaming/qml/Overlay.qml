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
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

import Apx.Common 1.0
import Apx.Controls 1.0

Item {
    id: control

    property bool interactive: false

    property var frameRect

    property var plugin: apx.tools.videostreaming
    property bool alive: true
    property bool loaded: plugin?true:false

    property bool showNumbers: true
    property bool showAim: true


    opacity: ui.effects?0.7:1
    MouseArea {
        anchors.fill: parent
        onClicked: control.forceActiveFocus()
    }

    Item {
        id: videoFrame
        x: (frameRect?frameRect.x:0)-control.x
        y: (frameRect?frameRect.y:0)-control.y
        width: frameRect?frameRect.width:parent.width
        height: frameRect?frameRect.height:parent.height
    }


    Loader {
        active: !alive
        anchors.centerIn: videoFrame
        sourceComponent: MaterialIcon {
            color: "#60FFFFFF"
            name: "video-off-outline"
            size: 64
        }
    }


    Loader {
        active: control.loaded && showNumbers
        anchors.fill: interactive?(loaded && apx.tools.videostreaming.tune.view_mode.value>0?control:videoFrame):control
        anchors.margins: 10
        sourceComponent: OverlayNumbers {
            id: numbers
            interactive: control.interactive
            alive: control.alive

            OverlayGimbal {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: numbers.overlayItemSize*1.1
                width: Math.min(80, parent.height/5)
            }
        }
    }

    property int aimSize: Math.max(60,Math.min(100, control.height/10))
    Loader {
        active: control.loaded && alive && showAim
        anchors.centerIn: videoFrame
        sourceComponent: OverlayAim {
            size: aimSize
            type: apx.tools.videostreaming.tune.overlay.aim.value
        }
    }

    Loader {
        active: control.loaded && interactive && apx.tools.videostreaming.tune.controls.value
        anchors.fill: videoFrame
        sourceComponent: CamControls {
            size: aimSize
        }
    }


    Connections {
        enabled: true //!interactive
        target: application
        function onLoadingFinished()
        {
            control.plugin=apx.tools.videostreaming
            control.loaded=true
            //console.log(control.plugin)
        }
    }


}
