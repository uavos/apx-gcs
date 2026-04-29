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

import APX.Facts

Fact {
    id: filterChooser

    function addSelected() {
        console.log("size:", filterChooser.size)
        for(var i = 0; i < filterChooser.size; ++i) {
            var fact = filterChooser.child(i)
            if(fact && fact.value)
                fMenu.createFilter(fact.name) 
        }
    }
    
    Fact {
        id: chrRunningAvg
        name: "running_avg"
        title: qsTr("Running average")
        descr: qsTr("Select running average filter")
        flags: Fact.Bool
        onTriggered: {
            fMenu.createFilter(name);
            filterChooser.menuBack()
        }
    }
    Fact {
        id: chrKalmanSimple
        name: "kalman_smp"
        title: qsTr("Kalman simple")
        descr: qsTr("Select simple kalman filter")
        flags: Fact.Bool
        onTriggered: {
            fMenu.createFilter(name)
            filterChooser.menuBack()
        }    
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Add selected")
        icon: "check-circle"
        onTriggered: {
            addSelected()
            filterChooser.menuBack()
        }
    }
}
