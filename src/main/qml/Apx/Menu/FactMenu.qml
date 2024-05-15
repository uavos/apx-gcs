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

import Apx.Common
// import Apx.Menu

import APX.Facts

import "."

StackView {
    id: stackView

    property var fact

    property bool showTitle: true
    property bool autoResize: false

    property bool effects: ui.effects

    property int maxEnumListSize: 5

    property int priority: 0

    Component.onCompleted: {
        Menu.registerMenuView(stackView)
        showFact(fact)
    }
    Component.onDestruction: {
        Menu.unregisterMenuView(stackView)
    }

    clip: true
    implicitWidth: currentItem?currentItem.implicitWidth:MenuStyle.itemWidth
    implicitHeight: currentItem?currentItem.implicitHeight:MenuStyle.itemWidth/3

    signal factButtonTriggered(var fact)
    signal factOpened(var fact)
    signal stackEmpty()
    signal closeTriggered()

    property bool showBtnBack: depth>1
    property bool showBtnClose: false


    //mandala select support
    property var mandalaFact: null
    onFactButtonTriggered: (fact) => {
        if(fact.dataType===Fact.Int && fact.units==="mandala"){
            mandalaFact=fact
            currentItem.pageTitle = mandalaFact.title+": "+qsTr("select")
            currentItem.pageStatus = mandalaFact.text
        }else if(mandalaFact && fact.treeType===Fact.NoFlags){
            mandalaFact.value=fact.mpath()
            while(mandalaFact)back()
        }
    }
    onFactChanged: {
        if(mandalaFact){
            if(mandalaFact===fact || mandalaFact.parentFact===fact){
                mandalaFact=null
            }
        }
    }
    function mandalaFactReset()
    {
        if(!mandalaFact) return
        mandalaFact.value=null;
        while(mandalaFact)back()
    }

    //menu.js helpers
    function showFact(f)
    {
        var opts={}
        opts.fact=f
        var c=pageDelegate.createObject(this, opts)
        stackView.fact=f
        push(c)
        forceActiveFocus()
        factOpened(f)
    }

    Component {
        id: pageDelegate
        FactMenuPage { }
    }


    function back()
    {
        //console.log("back",depth)
        if(depth==1)stackEmpty()
        if(depth>1)pop();
        stackView.fact=Qt.binding(function(){return currentItem.fact})
    }

    onCurrentItemChanged: _validCheckTimer.restart()
    Timer {
        id: _validCheckTimer
        interval: 1
        onTriggered: {
            for(var i=currentItem; i; i = stackView.get(i.StackView.index - 1)){
                if(i.valid) continue
                back()
                break
            }
        }
    }

    function openSystemTree()
    {
        clear()
        openFact(apx)
    }
}

