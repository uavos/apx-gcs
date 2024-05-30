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
import QtQuick.Layouts
import QtQml

import APX.Facts
import Apx.Common

ColumnLayout {
    id: menuPage

    //Fact bindings
    property var fact

    property bool pageInfo: false //load fact info instead of list
    property var pageInfoAction //load fact action info instead fact info

    property string pageTitle: fact?fact.title:""
    property string pageDescr: fact?fact.descr:""
    property string pageStatus: fact?fact.text:""

    property bool valid: fact?true:false

    Component.onCompleted: {
        pageLoader.source=pageSource()
    }

    StackView.onRemoved: {
        if(menuPage)menuPage.destroy()
    }

    onValidChanged: if(!valid) back()

    Component {
        id: factC
        Fact {}
    }
    Connections {
        enabled: valid
        target: fact
        function onMenuBack()
        {
            //console.log("menuBack")
            valid=false
        }
        function onRemoved()
        {
            //console.log("removed",fact)
            valid=false
            fact=factC.createObject(this)
        }
    }


    property real padding: Style.spacing
    clip: true

    property alias titleSize: titleItem.height

    Loader {
        id: titleItem
        active: valid
        source: "FactMenuPageTitle.qml"
        Layout.fillWidth: true
        Layout.leftMargin: padding
        Layout.rightMargin: padding
        Layout.topMargin: padding
        Layout.alignment: Qt.AlignTop|Qt.AlignHCenter
        clip: true
    }
    Loader {
        id: pageLoader
        active: valid
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: padding
        Layout.rightMargin: padding
        Layout.topMargin: padding
        clip: true
    }

    function pageSource()
    {
        if(fact.opts.page){
            var s=fact.opts.page
            if(s.indexOf(":")>=0){
                return s
            }
            return "../"+s
        }
        return "FactMenuPageList.qml"
    }

    function factButtonTriggered(fact)
    {
        if(factMenu)
            factMenu.factButtonTriggered(fact)
    }


    //actions
    RowLayout {
        id: actionsItem

        Layout.alignment: Qt.AlignRight
        Layout.bottomMargin: Style.spacing
        Layout.leftMargin: Style.spacing
        Layout.rightMargin: Style.spacing

        spacing: Style.spacing
        visible: repeater.count>0

        property alias model: repeater.model
        Repeater {
            id: repeater
            model: fact.actionsModel
            delegate: Loader{
                active: modelData && modelData.visible && ((modelData.options&Fact.ShowDisabled)?true:modelData.enabled)
                visible: active
                sourceComponent: Component {
                    ActionButton {
                        fact: modelData
                        onTriggered: menuPage.factButtonTriggered(modelData)
                    }
                }
            }
        }
    }

}
