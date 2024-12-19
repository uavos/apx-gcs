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
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQml

import APX.Facts

import Apx.Common

FactMenuPageList {
    id: control

    property var parentFact: fact

    delegate: Loader{
        // asynchronous: true
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
                    if(parentFact && parentFact.opts.dbtool){
                        parentFact.triggerItem(d)
                    }else{
                        parentFact.model.triggerItem(d.id)
                    }
                }
                onPressAndHold: {
                    console.log(JSON.stringify(d,' ',2))
                }
                // toolTip: JSON.stringify(d,' ',2)
            }
        }
    }
}
