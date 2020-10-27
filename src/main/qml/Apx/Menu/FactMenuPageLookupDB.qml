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
import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQml 2.12

import APX.Facts 1.0

import Apx.Common 1.0

FactMenuPageList {
    id: control

    property var parentFact: fact

    delegate: Loader{
        asynchronous: true
        active: true
        visible: active
        width: listView.width
        height: active?MenuStyle.itemSize:0
        sourceComponent: Component {
            FactButton {
                size: MenuStyle.itemSize
                property var d: modelData
                text: d.title?d.title:qsTr("No title")
                descr: d.descr?d.descr:""
                value: d.value?d.value:""
                active: d.active?d.active:false
                showEditor: false
                onTriggered: {
                    parentFact.triggerItem(modelData)
                }
            }
        }
    }
}
