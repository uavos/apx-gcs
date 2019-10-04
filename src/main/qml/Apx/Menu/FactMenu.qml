import QtQuick 2.6
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0
import Apx.Menu 1.0

import "."

StackView {
    id: factMenu

    property var fact

    property bool showTitle: true
    property bool autoResize: false

    property int titleRightMargin: 0

    property bool effects: ui.effects

    property int maxEnumListSize: 5

    property int priority: 0

    Component.onCompleted: {
        Menu.registerMenuView(factMenu)
        showFact(fact)
    }
    Component.onDestruction: {
        Menu.unregisterMenuView(factMenu)
    }

    clip: true
    implicitWidth: currentItem?currentItem.implicitWidth:MenuStyle.itemWidth
    implicitHeight: currentItem?currentItem.implicitHeight:MenuStyle.itemWidth/3

    signal factButtonTriggered(var fact)
    signal factOpened(var fact)
    signal stackEmpty()

    property bool showBtnBack: depth>1


    function showFact(f)
    {
        var c=pageDelegate.createObject(this, {"fact": f})
        fact=f
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
        //console.log("back")
        if(depth==1)stackEmpty()
        if(depth>1)pop();
        fact=currentItem.fact
    }
    Connections {
        target: factMenu.fact
        onMenuBack: {
            //console.log("menuBack")
            back()
        }
        onRemoved: {
            //console.log("removed")
            factMenu.fact=null
        }
    }


    function openSystemTree()
    {
        clear()
        openFact(apx)
    }
}

