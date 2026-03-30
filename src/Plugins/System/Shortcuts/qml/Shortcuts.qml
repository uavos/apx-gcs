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
import QtQml

import Apx.Common

Loader {

    property var plugin: apx.settings.interface.shortcuts
    active: plugin && !plugin.blocked.value

    sourceComponent: sys

    Loader {
        sourceComponent: usr
    }

    Component {
        id: sys
        Instantiator {
            model: plugin?plugin.system.model:null
            Shortcut {
                context: Qt.ApplicationShortcut
                enabled: modelData.enb.value
                sequences: [modelData.key.text]
                onActivated: application.jsexec(modelData.scr.text);
                Component.onCompleted: parent=ui.main
            }
        }
    }

    Component {
        id: usr
        Instantiator {
            model: plugin?plugin.user.model:null
            Shortcut {
                context: Qt.ApplicationShortcut
                enabled: modelData.enb.value
                sequences: [modelData.key.text]
                onActivated: application.jsexec(modelData.scr.text)
                Component.onCompleted: parent=ui.main
            }
        }
    }
}
