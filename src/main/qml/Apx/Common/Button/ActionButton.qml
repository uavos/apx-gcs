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
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import APX.Facts 1.0
import Apx.Common 1.0

ToolButton {
    id: control

    property QtObject fact

    property bool noFactTrigger: false

    property int treeType: fact?fact.treeType:Fact.NoFlags
    property int dataType: fact?fact.dataType:Fact.NoFlags
    property int options: fact?fact.options:Fact.NoFlags

    property string descr: fact?fact.descr:""

    iconName: fact?fact.icon:""
    text: fact?fact.title:""

    enabled: fact?fact.enabled:true
    visible: (fact?fact.visible:true) && (bShowDisabled || enabled)

    property bool active: fact?fact.active:false
    highlighted: activeFocus || active

    showText: !bIconOnly

    toolTip: descr


    //internal
    property bool bApply: dataType==Fact.Apply
    property bool bRemove: dataType==Fact.Remove
    property bool bStop: dataType==Fact.Stop

    property bool bIconOnly: options&Fact.IconOnly
    property bool bShowDisabled: options&Fact.ShowDisabled

    function action_color()
    {
        switch(dataType){
        case Fact.Apply: return Material.color(Material.Green)
        case Fact.Remove:
        case Fact.Stop: return Material.color(Material.Red)
        }
        return undefined
    }

    color: action_color()

    onTriggered: if(fact && !noFactTrigger) fact.trigger()
}
