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
import QtQuick.Controls 2.12


ListView {
    id: listView

    clip: true

    implicitWidth: contentItem.childrenRect.width

    signal filter(var text, var v)

    property var packets: ({})

    function pid(text, color)
    {
        //console.log(text, color)

        var p=packets[text]
        if(p){
            p.count++
            _model.setProperty(p.modelIndex,"count",p.count)
        }else{
            p={}
            p.text=text
            p.count=0
            p.color=color
            p.modelIndex=_model.count
            packets[text]=p
            _model.append(p)
        }
    }

    ListModel {
        id: _model
    }


    spacing: 2

    model: _model
    delegate: DatalinkInspectorItem {
        text: model.text + " ["+model.count+"]"
        itemColor: model.color
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if(invert){
                    model.count=0
                    packets[model.text].count=0;
                }

                invert=!invert
                filter(model.text, invert)
            }
        }
    }
}
