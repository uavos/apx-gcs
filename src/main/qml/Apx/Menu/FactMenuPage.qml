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
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQml 2.12

import APX.Facts 1.0
import Apx.Common 1.0

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


    property int padding: 4
    clip: true

    Loader {
        active: valid
        asynchronous: true
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
        asynchronous: true
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
}
