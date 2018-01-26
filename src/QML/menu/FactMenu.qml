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

    property bool showTitle: true
    property bool autoResize: false

    property int itemSize: 38
    property int titleSize: itemSize
    property int itemWidth: itemSize*8

    property int titleFontSize: itemSize*0.45
    property int descrFontSize: itemSize*0.3
    property int iconFontSize: itemSize*0.75
    property int editorFontSize: itemSize*0.5

    property bool effects: app.settings.smooth.value

    property int maxEnumListSize: 5

    clip: true
    initialItem: createFactPage(fact)

    implicitWidth: currentItem?currentItem.implicitWidth:0
    implicitHeight: currentItem?currentItem.implicitHeight:0

    signal opened()

    signal factTriggered(var fact)
    signal actionTriggered(var fact)

    signal factPageRemoved()

    property StackView parentStack
    property bool showBtnBack: depth>1 || parentStack


    function createFactPage(f,opts)
    {
        if(typeof opts==='undefined')opts={}
        opts.fact=f
        var c=pageDelegate.createObject(this,opts)
        f.removed.connect(function(){factPageRemoved(); back()})
        return c
    }

    function openFact(f,opts)
    {
        push(createFactPage(f,opts))
        opened()
    }

    /*function openPage(opts)
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
    }*/

    function back()
    {
        if(depth>1)pop();
        else if(parentStack)parentStack.pop();
    }

    function openSystemTree()
    {
        clear()
        openFact(app)
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

