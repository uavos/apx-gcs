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

ActionButton {
    id: control

    enabled: false
    visible: true

    showIcon: false

    textScale: 1

    property bool showValue: true

    property string value: fact?fact.text:""

    property bool warning: false
    property bool error: false
    property bool valueHighlight: false

    property bool alerts: false
    property bool alertsVehicle: true


    property color normalColor: "#222"
    property color activeColor: Material.color(Material.BlueGrey)
    property color warningColor: Qt.darker(Material.color(Material.Orange),1.5)
    property color errorColor: Material.color(Material.Red)

    property color valueColor: Material.primaryTextColor
    property color valueHighlightColor: "#30ffffff"

    property real valueScale: 1
    property real valueSize: height * valueScale

    font.family: font_narrow_regular

    color: {
        var c
        if(error)c=errorColor
        else if(warning)c=warningColor
        else if(active)c=activeColor
        else c=normalColor //Qt.lighter(normalColor,1.0+hoverFactor*2)
        return c //hoverFactor>0?Qt.lighter(c,1.0+hoverFactor):c
    }

    textColor: (active||warning||error)?Material.primaryTextColor:Qt.darker(Material.primaryTextColor,1.5)
    currentTextColor: textColor
    currentIconColor: iconColor

    function message() {
        var s=[]
        s.push(title+":")
        if(descr)s.push(descr)
        s.push("("+value+")")
        return s.join(" ")
    }

    property bool doAlerts: alerts && (apx.datalink.valid || apx.vehicles.current.protocol.isReplay)

    property var cv

    onWarningChanged: {
        if(!warning)return
        if(alertsVehicle && cv !== apx.vehicles.current){
            cv=apx.vehicles.current
            return
        }
        if(doAlerts){
            apx.vehicles.current.warnings.warning(message())
        }
    }
    onErrorChanged: {
        if(!error)return
        if(alertsVehicle && cv !== apx.vehicles.current){
            cv=apx.vehicles.current
            return
        }
        if(doAlerts){
            apx.vehicles.current.warnings.error(message())
        }
    }

    Component.onCompleted: {
        cv = apx.vehicles.current
        background.color=Qt.binding(function(){return Material.buttonColor})
    }

    //value

    Component {
        id: _valueC
        Text {
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            text: control.value
            font.family: font_narrow
            font.pixelSize: valueSize
            color: valueColor
            font.capitalization: Font.AllUppercase
        }
    }
    property Component valueC: _valueC


    contentComponent: Component {
        ValueContent {
            iconC: (control.showIcon && control.iconName)?control.iconC:null
            textC: (control.showText && control.text)?control.textC:null
            valueC: (control.showValue)?control.valueC:null
        }
    }

}
