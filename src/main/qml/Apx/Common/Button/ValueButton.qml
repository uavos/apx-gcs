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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import APX.Facts
import APX.Fleet as APX

import ".."

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
    property bool alertsUnit: true


    property color normalColor: "#222"
    property color activeColor: Material.color(Material.BlueGrey)
    property color warningColor: Qt.darker(Material.color(Material.Orange),1.5)
    property color errorColor: Material.color(Material.Red)

    property color valueColor: Material.primaryTextColor
    property color valueHighlightColor: "#30ffffff"

    property real valueScale: 1
    property real valueSize: (height>3?height*valueScale:3) 

    textBold: false
    
    readonly property APX.Unit unit: apx.fleet.current

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

    property bool doAlerts: alerts && (apx.datalink.valid || unit.isReplay)

    property var cv

    onWarningChanged: {
        if(!warning)return
        if(alertsUnit && cv !== unit){
            cv=unit
            return
        }
        if(doAlerts){
            unit.warnings.warning(message())
        }
    }
    onErrorChanged: {
        if(!error)return
        if(alertsUnit && cv !== unit){
            cv=unit
            return
        }
        if(doAlerts){
            unit.warnings.error(message())
        }
    }

    Component.onCompleted: {
        cv = unit
        // background.color=Material.buttonColor
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
