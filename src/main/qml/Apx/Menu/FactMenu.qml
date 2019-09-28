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
    //width: currentItem?currentItem.implicitWidth:0
    //height: currentItem?currentItem.implicitHeight:0

    //onImplicitHeightChanged: console.log(implicitHeight)

    /*property int iHeight: Math.max(currentItem?currentItem.implicitHeight:0,MenuStyle.itemWidth/3)
    onIHeightChanged: heightTimer.restart()
    Timer {
        id: heightTimer
        interval: 100
        onTriggered: implicitHeight=iHeight
    }*/


    signal factButtonTriggered(var fact)
    signal factOpened(var fact)
    signal stackEmpty()

    property StackView parentStack
    property bool showBtnBack: depth>1 || parentStack


    function showFact(f)
    {
        //console.log("showFact", f)
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
        else if(parentStack)parentStack.pop();
        fact=currentItem.fact
    }
    Connections {
        target: factMenu.fact
        onMenuBack: {
            //console.log("menuBack")
            back()
        }
    }


    function openSystemTree()
    {
        clear()
        openFact(apx)
    }


    function popItem(pageItem)
    {
        if(depth==1)stackEmpty()
        if(pageItem && factMenu.contains(pageItem)){
            pop(pageItem,StackView.Immediate);
        }
        back();
        //console.log("pop",depth,typeof(fact))
    }

}

