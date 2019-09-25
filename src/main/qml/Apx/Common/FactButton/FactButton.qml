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


    iconName: fact?fact.icon:""
    title: fact?fact.title:""
    descr: fact?fact.descr:""

    progress: fact?fact.progress:-1

    property string value: fact?fact.text:""
    property string status: fact?fact.status:""
    property bool active: fact?fact.active:false
    property bool modified: fact?fact.modified:false

    property int treeType: fact?fact.treeType:Fact.NoFlags
    property int dataType: fact?fact.dataType:Fact.NoFlags
    property int factSize: fact?fact.size:0
    property string qmlPage: fact?fact.qmlPage:""

    enabled: fact?fact.enabled:true

    property bool selected: false
    property bool draggable: true

    //toolTip: text+"\n"+descr

    showText: true
    textAlignment: Text.AlignLeft


    highlighted: activeFocus || selected

    titleColor: modified?Material.color(Material.Yellow):active?"#A5D6A7":Material.primaryTextColor


    property bool expandable: treeType !== Fact.Action
                              && (
                                  factSize>0
                                  || (treeType === Fact.Group)
                                  || (treeType === Fact.Page)
                                  || qmlPage
                                  || isMandala
                                  )
    property bool isMandala: dataType===Fact.Mandala
    property bool isScript: dataType===Fact.Script
    property bool hasValue: dataType && (!isScript)

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
        //if(typeof(listView)!='undefined')listView.currentIndex=index
        if(isScript) openDialog("EditorScript")
        else if(expandable && fact) openFact(fact.bind?fact.bind:fact)
        //else if(isAction)actionTriggered(fact)
        if(factTrigger && fact)fact.trigger()
    }

    onMenuRequested: {
        if(!draggable){
            if(fact)openFact(fact,{"pageInfo": true})
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
            if(fact)drag.source.fact.move(fact.num)
        }
    }



    property real statusSize: 0.5
    property real valueSize: 0.6
    property real nextSize: 0.7

    contents: [
        //status
        Label {
            text: factButton.status
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
        if(!fact)return ""
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
        if(!fact)return
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


