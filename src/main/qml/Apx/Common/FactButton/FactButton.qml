/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Material.impl 2.12


import APX.Facts 1.0
import Apx.Common 1.0

import "../Button"
import ".."

ActionButton {
    id: control

    onFactChanged: {
        if(!fact) fact=factC.createObject(this)
    }
    Component {
        id: factC
        Fact {}
    }

    property int progress: fact?fact.progress:-1

    showText: true
    bShowDisabled: true

    toolTip: {
        var list = []
        if(!fact){
            list.push(text)
            list.push(descr)
            return list.join("\n")
        }

        var s=fact.toolTip().trim()
        if(s) list.push(s)


        if(fact.section)
            list.push("section: "+fact.section)

        for(var i in fact.opts)
            list.push(i+": "+fact.opts[i])

        return list.join("\n")
    }


    property string value: fact?fact.text:""

    property bool modified: fact?fact.modified:false

    property int factSize: fact?fact.size:0
    property string factPage: (fact && fact.opts.page)?fact.opts.page:""

    property string units: fact?fact.units:""

    property bool selected: false
    property bool draggable: (fact && fact.parentFact)?fact.parentFact.options&Fact.DragChildren:false

    property bool signaled: false

    property bool noEdit: false


    readonly property bool opt_highlight: fact?fact.options&Fact.HighlightActive:true

    highlighted: activeFocus || selected || (active && opt_highlight)

    textColor: modified
               ? Material.color(Material.Yellow)
               : active
                 ? "#A5D6A7"
                 : Material.primaryTextColor


    property bool expandable: treeType !== Fact.Action
                              && (
                                  factSize>0
                                  || (treeType === Fact.Group)
                                  || factPage
                                  || isMandala
                                  )
    property bool isMandala: dataType === Fact.Int && units === "mandala"
    property bool isScript: dataType === Fact.Text && units === "script"
    property bool hasValue: dataType || value

    property bool showEditor: (!noEdit) && hasValue && showText && (!isScript)
    property bool showValue: hasValue && showText
    property bool showNext: expandable
    property bool showDescr: descr

    //focus requests
    signal focusRequested()
    signal focusFree()

    property bool bFocusRequest: false
    onFocusRequested: bFocusRequest=true
    onFocusFree: forceActiveFocus()

    property bool held: false

    onTriggered: {
        focusRequested()
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
        enabled: control.draggable
        anchors { fill: parent; margins: 10 }
        //keys: String(control.parent)
        onEntered: {
            //console.log(drag.source.title+" -> "+title)
            if(fact)drag.source.fact.move(fact.num)
        }
    }


    textSize: 0.55 * control.height

    property real titleSize: textSize * 1
    property real descrSize: textSize * 0.56


    property real statusSize: textSize * 1
    property real valueSize: textSize * 1
    property real nextSize: textSize * 1

    property color descrColor: Material.secondaryTextColor

    contentComponent: Component {
        Item {

            //BoundingRect{}

            id: titleLayout
            // anchors.fill: parent

            readonly property bool showIcon: control.showIcon && control.iconName
            readonly property bool showText: control.showText && control.text

            implicitWidth: (showIcon?_icon.implicitWidth:0)
                         + (showText?_titleText.implicitWidth+Style.spacing:0)
                         + (showNext?_next.implicitWidth+Style.spacing:0)
                         + (_value.item?_value.implicitWidth+Style.spacing:0)
                         + (_editor.item?_editor.implicitWidth+Style.spacing:0)
                         + Style.spacing

            Loader {
                id: _icon
                active: showIcon
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                sourceComponent: control.iconC
            }

            Text {
                id: _titleText
                anchors.left: _icon.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.leftMargin: Style.spacing
                visible: showText
                verticalAlignment: _descrText.visible?Text.AlignTop:Text.AlignVCenter
                font.family: control.font.family
                font.pointSize: titleSize
                text: control.text
                color: control.enabled?textColor:disabledTextColor
            }
            Text {
                id: _descrText
                anchors.left: _icon.right
                anchors.right: _next.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.leftMargin: Style.spacing
                anchors.rightMargin: _value.valueWidth + _editor.editorWidth
                visible: _titleText.visible && showDescr && text
                verticalAlignment: Text.AlignBottom
                font: apx.font_condenced(descrSize)
                text: control.descr
                color: control.enabled?descrColor:disabledTextColor
                elide: Text.ElideMiddle
                clip: true
            }

            // next icon
            MaterialIcon {
                id: _next
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                visible: showNext
                size: nextSize
                verticalAlignment: Text.AlignVCenter
                name: "chevron-right"
                color: control.enabled?Material.secondaryTextColor:Material.hintTextColor
            }

            // value
            Item {
                anchors.left: _titleText.right
                anchors.right: _next.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                Loader {
                    id: _value
                    active: showValue && (!_editor.item)
                    anchors.fill: parent
                    readonly property real valueWidth: item
                            ? (item.truncated
                                ? item.width
                                : implicitWidth
                            )
                            : 0
                    sourceComponent: Text {
                        id: textItem
                        text: (value.length>64||value.indexOf("\n")>=0)?"<data>":value
                        font: apx.font_condenced(valueSize)
                        color: Material.secondaryTextColor
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideMiddle
                    }
                }
                Loader {
                    id: _editor
                    asynchronous: true
                    active: source
                    Material.accent: Material.color(Material.Green)
                    source: showEditor?getEditorSource():""
                    anchors.fill: parent
                    // anchors.leftMargin: Style.spacing
                    anchors.leftMargin: Math.max(0,parent.width-(item?item.implicitWidth:0))

                    readonly property real editorWidth: item?item.implicitWidth:0
                }
            }

        }
    }

    Loader {
        parent: contentItem
        z: -1
        asynchronous: true
        active: control.progress>=0
        anchors.fill: parent
        anchors.margins: 1
        sourceComponent: Component {
            ProgressBar {
                background.height: height-2
                contentItem.implicitHeight: height-2
                opacity: 0.7
                to: 100
                property int v: control.progress
                value: v
                visible: v>=0
                indeterminate: v==0
                Material.accent: Material.color(Material.Green)
            }
        }
    }

    function getEditorSource()
    {
        if(!fact)
            return ""

        if(fact.opts.editor)
            return fact.opts.editor

        var qml
        switch(fact.dataType){
        case Fact.Bool:
            qml="Switch"
            break
        case Fact.Text:
            if(fact.enumStrings.length > 0) qml="TextOption"
            else qml="Text"
            break
        case Fact.Enum:
            qml="Option"
            break
        case Fact.Int:
            if(units==="time")qml="Time"
            else if(units==="mandala")break
            else qml="Int"
            break
        case Fact.Float:
            if(units==="lat" || units==="lon")qml="Text"
            else qml="Float"
            break
        }
        if(!qml)return ""
        return "Editor"+qml+".qml"
    }

    function openDialog(name)
    {
        if(!fact)return
        var c=Qt.createComponent(name+".qml",control)
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
                anchor: control
                active: false
                color: control.flat && control.highlighted ? control.Material.highlightedRippleColor : control.Material.rippleColor
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

}


