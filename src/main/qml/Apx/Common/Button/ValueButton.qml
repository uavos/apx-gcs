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
import APX.Vehicles 1.0 as APX

ActionButton {
    id: control

    enabled: false
    visible: true

    showIcon: false

    textScale: 0.8

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

    textBold: false
    
    readonly property APX.Vehicle vehicle: apx.vehicles.current

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
        if(text)
            s.push(text+":")
        if(descr)
            s.push(descr)
        s.push("("+value+")")
        return s.join(" ")
    }

    property bool doAlerts: alerts && (apx.datalink.valid || vehicle.isReplay)

    property var cv

    onWarningChanged: {
        if(!warning)return
        if(alertsVehicle && cv !== vehicle){
            cv=vehicle
            return
        }
        if(doAlerts){
            vehicle.warnings.warning(message())
        }
    }
    onErrorChanged: {
        if(!error)return
        if(alertsVehicle && cv !== vehicle){
            cv=vehicle
            return
        }
        if(doAlerts){
            vehicle.warnings.error(message())
        }
    }

    Component.onCompleted: {
        cv = vehicle
        background.color=Qt.binding(function(){return Material.buttonColor})
    }

    //value

    Component {
        id: _valueC
        Text {
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            text: control.value.toUpperCase()
            font: apx.font_narrow(valueSize)
            color: valueColor
        }
    }
    property Component valueC: _valueC


    contentComponent: Component {
        Item {
            implicitWidth: _titleRow.implicitWidth + _valueItem.implicitWidth + Style.spacing
            implicitHeight: Style.buttonSize

            property real iconWidth: _icon.item?_icon.width:0
            property real titleWidth: _title.item?_title.width:0

            Row {
                id: _titleRow
                spacing: 0
                anchors.fill: parent

                // icon
                Loader {
                    id: _icon
                    height: parent.height
                    width: item?height:0
                    active: control.showIcon && control.iconName
                    sourceComponent: control.iconC
                }

                // title
                Loader {
                    id: _title
                    height: parent.height
                    active: control.showText && control.text
                    sourceComponent: control.textC
                }
            }
            
            Loader {
                id: _valueItem
                anchors.fill: parent
                anchors.leftMargin: iconWidth + titleWidth + Style.spacing
                active: control.showValue
                sourceComponent: control.valueC
            }
        }
    }

}
