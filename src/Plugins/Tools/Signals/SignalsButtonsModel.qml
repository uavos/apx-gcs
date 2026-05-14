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
    id: buttonsModel

    function addButton(fact) {
        if(!fact)
            return;
        var component = Qt.createComponent("SignalsButton.qml");
        if (component.status === Component.Ready) {
            var c=component.createObject(bottomArea, {"pageFact": fact});
            buttonsModel.append(c);
        }
    }

    function removeButton(num) {
        for(var i = 0; i < buttonsModel.count; ++i) {
            var btn = buttonsModel.get(i);
            if(btn.num !== num)
                continue;
            buttonsModel.remove(i);
        }

        if(buttonsModel.count === 0)
            return;

        var index = 0;    
        for(var i = 0; i < buttonsModel.count; ++i) {
            if (buttonsModel.get(i).checked){ 
                index = i;
            }
        }
        buttonsModel.get(index).checked = true;
    }

    function updateModel(pageList) {
        // for(var i = 0; i < buttonsModel.count; ++i) {
        //     var btn =  buttonsModel.get(i);
        //     buttonsModel.remove(i);
        //     btn.destroy();
        // }
        clearModel();
        for(var i = 0; i < pageList.length; ++i) {
            var f = pageList[i]
            if(!f)
                continue;
            buttonsModel.addButton(f)
        }
    }

    function clearModel() {
        buttonsModel.clear();
        sgControl.clearButtonGroup(); // ButtonGroup keep model element after clear
    }
}
