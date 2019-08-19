import QtQuick 2.6
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

StackView {
    id: factMenu

    property var fact

    property bool showTitle: true
    property bool autoResize: false

    property int titleRightMargin: 0

    property bool effects: ui.effects

    property int maxEnumListSize: 5

    property var currentFact: fact

    /*onFactChanged: {
        clear()
        openFact(fact)
    }*/

    clip: true
    initialItem: createFactPage(fact)

    implicitWidth: currentItem?currentItem.implicitWidth:MenuStyle.itemWidth
    //implicitHeight: currentItem?currentItem.implicitHeight:MenuStyle.itemWidth/3
    //width: currentItem?currentItem.implicitWidth:0
    //height: currentItem?currentItem.implicitHeight:0

    //onImplicitHeightChanged: console.log(implicitHeight)

    property int iHeight: Math.max(currentItem?currentItem.implicitHeight:0,MenuStyle.itemWidth/3)
    onIHeightChanged: heightTimer.restart()
    Timer {
        id: heightTimer
        interval: 100
        onTriggered: implicitHeight=iHeight
    }

    signal opened()

    //signal factTriggered(var fact)
    signal factActionTriggered()
    signal stackEmpty()

    //signal factRemoved()

    property StackView parentStack
    property bool showBtnBack: depth>1 || parentStack


    function createFactPage(f,opts)
    {
        if(typeof opts==='undefined')opts={}
        if(!opts.fact)opts.fact=f
        var c=pageDelegate.createObject(this,opts)
        currentFact=f
        return c
    }

    function openFact(f,opts)
    {
        //console.log(f,opts)
        push(createFactPage(f,opts))
        opened()
    }

    function back()
    {
        //console.log("back")
        if(depth==1)stackEmpty()
        if(depth>1)pop();
        else if(parentStack)parentStack.pop();
        currentFact=currentItem.fact
    }

    function openSystemTree()
    {
        clear()
        openFact(apx)
    }

    Component {
        id: pageDelegate
        FactMenuPage { }
    }


    function popItem(pageItem)
    {
        if(depth==1)stackEmpty()
        if(pageItem && factMenu.contains(pageItem)){
            pop(pageItem,StackView.Immediate);
        }
        pop();
        //console.log("pop",depth,typeof(fact))
    }

}

