.pragma library
.import QtQuick.Controls 2.4 as Controls
.import QtQml 2.11 as Qml


var popupItem = null
var menuViews = new Set()
var menuPopups = new Set()

function show(fact,opts,parent)
{
    if(!fact) return
    var factMenu=fact.menu()
    if(!factMenu) return

    if(!opts) opts={}
    if(!parent) parent=ui.window

    //console.log("Menu.show", fact, JSON.stringify(opts), parent)

    var av=Array.from(menuViews).sort((a,b) => b.priority-a.priority)
    for(var menuItem of av){
        if(menuItem){
            //console.log(menuItem.priority)
            if(menuItem.fact === factMenu
                    || menuItem.fact === fact
                    || menuItem.fact.bind === factMenu
                    || menuItem.fact.bind === fact){
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

    var obj = component.createObject(parent, opts);

    if (obj === null || obj.status === Qml.Component.Error) {
        console.log("Error creating object:", obj.errorString());
        return
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
        while(z <= p.z)
            p.z-=0.01
    }
    //popup.z = z
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
