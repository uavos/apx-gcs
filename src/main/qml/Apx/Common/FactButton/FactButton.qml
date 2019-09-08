import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2

import APX.Facts 1.0
import Apx.Common 1.0

CleanButton {
    id: factButton
    property var fact


    onFactChanged: {
        if(!fact)fact=factC.createObject(this)
    }
    Component {
        id: factC
        Fact {}
    }


    iconName: fact.icon
    title: fact.title
    descr: fact.descr

    progress: fact.progress

    property string value: fact.text

    property bool active: fact.active
    property bool selected: false
    property bool draggable: true

    //toolTip: text+"\n"+descr

    showText: true
    textAlignment: Text.AlignLeft

    enabled: fact.enabled

    highlighted: activeFocus || selected

    titleColor: fact.modified?Material.color(Material.Yellow):active?"#A5D6A7":Material.primaryTextColor

    property bool expandable: fact.treeType !== Fact.Action && (fact.size || (fact.treeType === Fact.Group) || isMandala)
    //property bool isAction: fact.dataType===Fact.Action
    //property bool isFact: fact.treeType===Fact.FactItem
    property bool isMandala: fact.dataType===Fact.Mandala
    property bool isScript: fact.dataType===Fact.Script
    property bool hasValue: fact.dataType && (!isScript)

    property bool showEditor: hasValue
    property bool showValue: hasValue
    property bool factTrigger: true

    //focus requests
    signal focusRequested()
    signal focusFree()
    focus: false
    property bool bFocusRequest: false
    onFocusRequested: bFocusRequest=true
    onFocusFree: forceActiveFocus()

    property bool held: false

    onTriggered: {
        focusRequested()
        //if(!activeFocus)return
        if(typeof(listView)!='undefined')listView.currentIndex=index
        if(isScript) openDialog("EditorScript")
        else if(expandable) openFact(fact)
        //else if(isAction)actionTriggered(fact)
        if(factTrigger)fact.trigger()
    }

    onMenuRequested: {
        if(!draggable){
            openFact(fact,{"pageInfo": true})
        }else{
            held = true
        }
    }

    onPressed: grabToImage(function(result) {Drag.imageSource = result.url})
    onReleased: held = false

    Drag.dragType: Drag.Automatic
    Drag.active: held
    Drag.hotSpot.x: 10
    Drag.hotSpot.y: 10
    Drag.supportedActions: Qt.MoveAction
    //Drag.keys: String(parent) //fact.parentFact?fact.parentFact.name:"root"

    DropArea {
        anchors { fill: parent; margins: 10 }
        //keys: String(factButton.parent)
        onEntered: {
            console.log(drag.source.title+" -> "+title)
            //console.log(drag.target.title)
            //fact.ParentFact.model.move(drag.source.fact.num,fact.num)
            drag.source.fact.move(fact.num)
        }
    }



    property real statusSize: 0.5
    property real valueSize: 0.6
    property real nextSize: 0.7

    contents: [
        //status
        Label {
            text: fact.status
            font.family: font_fixed
            font.pixelSize: fontSize(bodyHeight*statusSize)
            color: factButton.enabled?Material.secondaryTextColor:Material.hintTextColor
        },
        //value
        Loader {
            active: showValue && (!editorItem.active)
            //Layout.fillHeight: true
            //Layout.maximumHeight: size
            sourceComponent: Component {
                Label {
                    text: (value.length>64||value.indexOf("\n")>=0)?"<data>":value
                    font.family: font_condenced
                    font.pixelSize: fontSize(bodyHeight*valueSize)
                    color: Material.secondaryTextColor
                }
            }
        },
        Loader {
            id: editorItem
            Layout.maximumHeight: bodyHeight
            Layout.maximumWidth: factButton.height*10
            Layout.rightMargin: 4
            asynchronous: true
            Material.accent: Material.color(Material.Green)
            property string src: showEditor?getEditorSource():""
            active: src
            visible: active
            source: src
        },
        //next icon
        Text {
            visible: expandable
            Layout.maximumWidth: font.pixelSize*0.7
            Layout.leftMargin: -font.pixelSize*0.25
            font.family: "Material Design Icons"
            font.pixelSize: fontSize(bodyHeight*nextSize)
            verticalAlignment: Text.AlignVCenter
            height: bodyHeight
            text: visible?materialIconChar["chevron-right"]:""
            color: factButton.enabled?Material.secondaryTextColor:Material.hintTextColor
        }
    ]


    function getEditorSource()
    {
        var qml=""
        switch(fact.dataType){
        case Fact.Bool:
            qml="Switch"
            break
        case Fact.Text:
            if(fact.enumStrings.length > 0)qml="TextOption"
            else qml="Text"
            break
        case Fact.Enum:
            qml="Option"
            break
        case Fact.Key:
            qml="Key"
            break
        case Fact.Int:
            if(fact.units==="time")qml="Time"
            else qml="Int"
            break
        case Fact.Float:
            if(fact.units==="lat" || fact.units==="lon")qml="Text"
            else qml="Float"
            break
        case Fact.Script:
            qml=""
            break
        }
        if(!qml)return ""
        return "Editor"+qml+".qml"
    }

    function openDialog(name)
    {
        var c=Qt.createComponent(name+".qml",factButton)
        if(c.status===Component.Ready) {
            c.createObject(ui.window,{"fact": fact});
        }
    }


    /*MouseArea {
        anchors.fill: factButton
        propagateComposedEvents: true
        drag.target: held ? factButton : undefined
        drag.axis: Drag.YAxis
    }*/
}


