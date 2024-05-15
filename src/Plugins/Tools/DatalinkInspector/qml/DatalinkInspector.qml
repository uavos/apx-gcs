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
import QtQuick.Controls

import Apx.Common


Rectangle {
    id: control

    property var plugin_fact: apx.tools.datalinkinspector

    implicitWidth: Style.buttonSize*25
    implicitHeight: Style.buttonSize*10

    border.width: 0
    color: "#000"

    RowLayout {
        anchors.fill: parent
        anchors.margins: Style.spacing

        DatalinkInspectorView {
            id: _view
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        DatalinkInspectorFilter {
            id: _filter
            Layout.fillHeight: true
            onFilter: plugin_fact.filter(uid,exclude)
        }


    }

    Component.onCompleted: {
        plugin_fact.active=true;
    }
    Component.onDestruction: {
        plugin_fact.active=false;
    }
}
