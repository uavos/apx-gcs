import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Material.impl 2.12


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

    toolTip: {
        var list = []
        if(!fact){
            list.push(title)
            list.push(descr)
            return list.join("\n")
        }
        list = fact.info().split("\n")

        if(fact.section)
            list.push("section: "+fact.section)

        for(var i in fact.opts)
            list.push(i+": "+fact.opts[i])

        return list.join("\n")
    }


    property string value: fact?fact.text:""
    property bool active: fact?fact.active:false
    property bool modified: fact?fact.modified:false

    property int treeType: fact?fact.treeType:Fact.NoFlags
    property int dataType: fact?fact.dataType:Fact.NoFlags

    property int factSize: fact?fact.size:0
    property string qmlPage: fact?fact.qmlPage:""

    enabled: fact?fact.enabled:true

    property bool selected: false
    property bool draggable: (fact && fact.parentFact)?fact.parentFact.options&Fact.DragChildren:false

    property bool signaled: false

    property bool noFactTrigger: false
    property bool noEdit: false

    showText: true
    textAlignment: Text.AlignLeft

    readonly property bool opt_highlight: fact?fact.options&Fact.HighlightActive:true

    highlighted: activeFocus || selected || (active && opt_highlight)

    titleColor: modified?Material.color(Material.Yellow):active?"#A5D6A7":Material.primaryTextColor


    property bool expandable: treeType !== Fact.Action
                              && (
                                  factSize>0
                                  || (treeType === Fact.Group)
                                  || qmlPage
                                  || isMandala
                                  )
    property bool isMandala: dataType===Fact.MandalaID
    property bool isScript: dataType===Fact.Script
    property bool hasValue: dataType || value

    property bool showEditor: (!noEdit) && hasValue && showText && (!isScript)
    property bool showValue: hasValue && showText
    property bool showNext: expandable

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
        if(noFactTrigger) return
        if(fact)fact.trigger()
        if(isScript) openDialog("EditorScript")
    }

    onPressAndHold: held = draggable
    onPressed: {
        if(draggable)
            grabToImage(function(result) {Drag.imageSource = result.url})
    }
    onReleased: held = false

    Drag.dragType: draggable?Drag.Automatic:Drag.None
    Drag.active: draggable && held
    Drag.hotSpot.x: 10
    Drag.hotSpot.y: 10
    Drag.supportedActions: Qt.MoveAction
    //Drag.keys: String(parent) //fact.parentFact?fact.parentFact.name:"root"

    DropArea {
        enabled: factButton.draggable
        anchors { fill: parent; margins: 10 }
        //keys: String(factButton.parent)
        onEntered: {
            //console.log(drag.source.title+" -> "+title)
            if(fact)drag.source.fact.move(fact.num)
        }
    }



    property real statusSize: 0.5
    property real valueSize: 0.6
    property real nextSize: 0.7

    contents: [
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
        Loader {
            id: nextIcon
            Layout.maximumWidth: size*0.7
            Layout.leftMargin: -size*0.25
            Layout.preferredHeight: bodyHeight
            active: showNext
            visible: active
            asynchronous: true
            property int size: fontSize(bodyHeight*nextSize)
            sourceComponent: Component {
                MaterialIcon {
                    size: nextIcon.size
                    verticalAlignment: Text.AlignVCenter
                    name: "chevron-right"
                    color: factButton.enabled?Material.secondaryTextColor:Material.hintTextColor
                }
            }
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

    Loader {
        anchors.fill: parent
        active: signaled
        asynchronous: true
        sourceComponent: Component {
            Ripple {
                id: ripple
                clipRadius: 2
                anchor: factButton
                active: false
                color: factButton.flat && factButton.highlighted ? factButton.Material.highlightedRippleColor : factButton.Material.rippleColor
                Timer {
                    interval: 500
                    repeat: true
                    running: true
                    onTriggered: {
                        ripple.pressed=!ripple.pressed
                    }
                }
            }
        }
    }


    /*MouseArea {
        anchors.fill: factButton
        propagateComposedEvents: true
        drag.target: held ? factButton : undefined
        drag.axis: Drag.YAxis
    }*/
}


