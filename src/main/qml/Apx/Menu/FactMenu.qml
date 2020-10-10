import QtQuick 2.6
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0
import Apx.Menu 1.0

import APX.Facts 1.0

import "."

StackView {
    id: stackView

    property var fact

    property bool showTitle: true
    property bool autoResize: false

    property int titleRightMargin: 0

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

    property bool showBtnBack: depth>1


    //mandala select support
    property var mandalaFact: null
    onFactButtonTriggered: {
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

