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
import QtQml.Models
import QtQuick.Layouts

import Apx.Common

ObjectModel {
    id: pinnedModel

    function addPage(fact) {
        if(!fact)
            return;
        var component = Qt.createComponent("SignalsChartItem.qml");
        if (component.status === Component.Ready) {
            var c=component.createObject(layout, {"Layout.fillWidth": true,
                                                "Layout.preferredHeight": Qt.binding(function(){return 110 * ui.scale}),
                                                "Layout.minimumHeight": 20,
                                                "clip": true});
            c.allowReset();
            c.ciPageFact = fact;
            pinnedModel.append(c);
        }
    }

    function removePage(num) {
        for(var i = 0; i < pinnedModel.count; ++i) {
            if(pinnedModel.get(i).num !== num)
                continue;
            pinnedModel.remove(i)
        }
    }

    function updateModel(pageList) {
        pinnedModel.clear();
        for(var i = 0; i < pageList.length; ++i) {
            var f = pageList[i]
            if(!f)
                continue;
            pinnedModel.addPage(f)
        }
    }
}
