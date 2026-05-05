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
    property bool changes: false

    onValueChanged: updateDescr()
    onChangesChanged: { if (changes) mChart.changes = true;}
    Component.onCompleted: {
        if (!value || value === undefined) 
            value = qsTr("Auto")
        console.log("Component created", value)
            
        var opt = opts;
        opt.page = "qrc:/FilteredCharts/SignalsColorChooser.qml";
        opts = opt;
    }

    function updateDescr() {
        var descrText = "";
        if(!value || value === undefined)
            descrText = qsTr("Auto")
        else
            descrText = value
        descr = qsTr("COLOR") + ": " + descrText.toString().toUpperCase();
        changes = true;
    }
}
