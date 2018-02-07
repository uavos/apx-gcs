import QtQuick 2.6
import GCS.FactSystem 1.0
import QtQml.Models 2.2
import "."

Item {
    property var fact
    property string title: fact?fact.title:""
    property int progress: fact?fact.progress:0
    property var model: fact?fact.model:children

    visible: fact?fact.visible:true

    property int size: fact?fact.size:children.length
    property int treeItemType: fact?fact.treeItemType:((size||enumStringsSize)?Fact.GroupItem:Fact.FactItem)
    property int dataType: fact?fact.dataType:(enumStringsSize?Fact.EnumData:Fact.ConstData)

    property string descr: showDescr?(fact?fact.descr:""):""
    property string text: fact?fact.text:""
    property string status: fact?fact.status:""

    property string iconSource: fact?fact.icon:""
    property string qmlPage: fact?fact.qmlPage:""

    property bool busy: fact?fact.busy:false
    property bool active: fact?fact.active:false
    property bool enabled: fact?fact.enabled:(title?true:false)


    property bool showDescr: false

    property bool isNotFact: true

    property var m_value
    readonly property var value: fact?fact.value:m_value

    function setValue(v){
        if(fact)fact.setValue(v)
        else valueUpdated(v)
    }
    signal valueUpdated(var v)

    /*onValueChanged: {
        if(fact){
            fact.value=value
            value = Qt.binding(function() { return fact.value })
            valueConnections.enabled=true
        }
        //console.log(fact)
    }
    Component.onCompleted: {
        if(fact){
            value = Qt.binding(function() { return fact.value })
        }
    }
    Connections {
        id: valueConnections
        enabled: false
    }*/


    signal removed()
    signal triggered()
    signal actionTriggered()
    signal toggled()

    function trigger()
    {
        triggered();
    }

    property var enumStrings: fact?fact.enumStrings:{}
    property var enumStringsSize: enumStrings?enumStrings.length:0

    /*property var enumList: Repeater {
        id: instantiator
        onObjectAdded: objModel.children.push(object)
        onObjectRemoved: objModel.remove(index,1)
        model: enumStrings
        //delegate: FactMenuElement { title: modelData; fact: undefined; enumList: undefined; onTriggered: { fact.value=modelData; }}
        delegate: enumC
    }*/

    /*Component {
        id: enumC
        FactMenuElement { title: modelData; fact: undefined; enumList: undefined; onTriggered: { fact.value=modelData; }}
    }*/


    function childFact(i)
    {
        return children[i]
    }






    default property alias children: contentsItem.children

    Item {
        id: contentsItem
    }
}
