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
.pragma library
.import QtQuick.Controls 2.4 as Controls
.import QtQml 2.11 as Qml


var popupItem = null
var menuViews = new Set()
var menuPopups = new Set()

// application calls this function to show menu popup
function show(fact,opts,parent)
{
    if(!fact) return
    var factMenu=fact.menu
    if(!factMenu) return

    if(!opts) opts={}
    if (!parent) parent = application.window
    
    // console.log("Menu.show", fact, JSON.stringify(opts), parent)

    var av=Array.from(menuViews).sort((a,b) => b.priority-a.priority)
    for(var menuItem of av){
        if(menuItem){
            //console.log(menuItem.priority)
            if(menuItem.fact === factMenu
                    || menuItem.fact === fact
                    || menuItem.fact.binding === factMenu
                    || menuItem.fact.binding === fact){
                //already displaying fact
                //console.log("Menu.skip", fact)
                menuItem.factOpened(menuItem.fact)
                return
            }
            if(fact.hasParent(menuItem.fact) || factMenu.hasParent(menuItem.fact)){
                //if(fact.parentFact==menuItem.fact || factMenu.parentFact==menuItem.fact){
                //console.log("Menu.update", menuItem)
                //console.log(factMenu.path())
                menuItem.showFact(factMenu)
                return
            }
        }
    }

    opts.fact = factMenu

    for(var opt in factMenu.opts){
        // console.log(opt)
        if(opts[opt]) continue
        opts[opt] = factMenu.opts[opt]
    }

    var component = Qt.createComponent("FactMenuPopup.qml",Qml.Component.Asynchronous,parent);
    if (component.status === Qml.Component.Ready){
        createMenuObject(component, opts, parent);
    }else{
        component.statusChanged.connect(function(){createMenuObject(component, opts, parent)});
    }
}

function createMenuObject(component, opts, parent)
{
    if (component.status === Qml.Component.Error) {
        console.log("Error loading component:", component.errorString());
        return
    }
    if (component.status !== Qml.Component.Ready) return;

    var obj = component.createObject(parent,{ fact: opts.fact });

    if (obj === null || obj.status === Qml.Component.Error) {
        console.log("Error creating object:", obj.errorString());
        return
    }

    // apply options
    for(var opt in opts){
        // console.log(opt)
        if(!obj.hasOwnProperty(opt)) continue
        obj[opt] = opts[opt]
    }

    //close all unpinned popups
    for(var p of menuPopups){
        if(p === obj)continue
        if(p.pinned)continue
        p.close()
    }

    obj.open()
}

function raisePopup(popup)
{
    menuPopups.delete(null)
    var z = popup.z
    for(var p of menuPopups){
        if(p === popup)continue
        p.menuEnabled=false
        if (z < p.z)
            z = p.z
    }
    popup.z = z+0.001
    popup.menuEnabled=true
    //console.log(z)
}
function activatePopup()
{
    menuPopups.delete(null)
    if(menuPopups.length<=0)return
    //activate highest z popup
    var popup=menuPopups.values().next().value
    for(var p of menuPopups){
        if(!p)continue
        p.menuEnabled=false
        if(popup.z>p.z)continue
        popup=p
    }
    if(popup)
        popup.menuEnabled=true
}

function registerMenuView(menu)
{
    //console.log("MENU:", menu)
    menuViews.add(menu)
}
function unregisterMenuView(menu)
{
    //console.log("MENU REMOVE:", menu)
    menuViews.delete(menu)
}
function registerMenuPopup(popup)
{
    menuPopups.add(popup)
}
function unregisterMenuPopup(popup)
{
    menuPopups.delete(popup)
    activatePopup()
}
