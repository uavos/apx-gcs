import QtQuick 2.6
import QtQml.Models 2.2

import APX.Facts 1.0

Item {
    property var fact
    property string title: fact?fact.title:""
    property int progress: fact?fact.progress:-1
    property var model: fact?fact.model:children
    property var actionsModel: fact?fact.actionsModel:actions

    visible: fact?fact.visible:true

    property int size: fact?fact.size:children.length
    property int treeType: fact?fact.treeType:(size?Fact.Group:Fact.NoFlags)
    property int dataType: fact?fact.dataType:(enumStringsSize?Fact.Enum:Fact.Const)

    property int flags: 0   //FactAction

    property string descr: showDescr?(fact?fact.descr:""):""
    property string text: fact?fact.text:value?value:""
    property string status: fact?fact.status:""

    property string section: fact?fact.section:""

    property string icon: fact?fact.icon:""
    property string qmlPage: fact?fact.qmlPage:""

    property bool active: fact?fact.active:false
    property bool enabled: fact?fact.enabled:(title?true:false)
    property bool modified: fact?fact.modified:false

    property var modelData: this

    property bool showDescr: true

    property bool isNotFact: true

    property var m_value
    readonly property var value: fact?fact.value:m_value

    function setValue(v){
        if(fact)fact.setValue(v)
        else {
            m_value=v
            valueUpdated(v)
        }
    }
    signal valueUpdated(var v)


    function info(){return ""}

    signal removed()
    signal triggered()
    signal actionTriggered()
    signal menuBack()
    signal toggled()

    function trigger()
    {
        triggered();
    }

    property var enumStrings: fact?fact.enumStrings:{}
    property var enumStringsSize: enumStrings?enumStrings.length:0

    function childFact(i)
    {
        return children[i]
    }


    function add(item)
    {
        contentsItem.children.push(item)
    }


    default property alias children: contentsItem.children
    Item {
        id: contentsItem
    }

    function clearChildren(skip)
    {
        var nlist=[]
        for(var i=0;i<skip;++i){
            nlist.push(children[i])
        }
        var list=children
        children=nlist
        for(i in list){
            if(i<skip)continue
            list[i].destroy()
        }
    }


    property alias actions: actionsItem.children
    Item {
        id: actionsItem
    }
}
