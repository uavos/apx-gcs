import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0
import "../components"
import "."

StackView {
    id: root

    property var fact

    property bool effects: true

    property bool autoResize: false

    property bool showTitle: true

    property int itemSize: 42
    property int maxEnumListSize: 5
    property int btnSize: itemSize*0.9
    property int titleSize: itemSize
    property int itemWidth: itemSize*10

    clip: true
    initialItem: createFactPage(fact)

    implicitWidth: currentItem.implicitWidth //currentItem?currentItem.implicitWidth:100
    implicitHeight: currentItem.implicitHeight //currentItem?currentItem.implicitHeight:100

    /*onImplicitHeightChanged: console.log(implicitHeight)
    onCurrentItemChanged: {
        console.log(currentItem)
        console.log(implicitHeight)
    }*/

    signal opened()

    signal factTriggered(var fact)
    signal actionTriggered(var fact)

    signal factPageRemoved()

    property StackView parentStack
    property bool showBtnBack: depth>1 || parentStack


    function createFactPage(f)
    {
        var c=pageDelegate.createObject(this,{"fact": f})
        f.removed.connect(function(){factPageRemoved(); back()})
        return c
    }

    function openFact(f)
    {
        push(createFactPage(f))
        opened()
    }

    function openPage(opts)
    {
        push(pageDelegate.createObject(this,opts))
        opened()
    }

    function pushUrl(url,opts)
    {
        push(Qt.resolvedUrl("../"+url),opts?opts:{"parentStack": this})
    }

    function close()
    {
        pop(null);
        closed();
    }

    function back()
    {
        if(depth>1)pop();
        else if(parentStack)parentStack.pop();
    }




    function isActionFact(f)
    {
        return (f.dataType===Fact.ActionData && (!f.isNotFact) && (f.value>=Fact.ButtonAction) )?true:false
    }


    Component {
        id: pageDelegate
        FactMenuPage { }
    }


}

