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
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls


Rectangle{
    id: consoleExec

    implicitHeight: cmdText.implicitHeight
    implicitWidth: cmdText.implicitWidth
    border.width: 0
    color: cmdText.activeFocus?"#133":"#111"
    readonly property string user: apx.fleet.current.title
    readonly property string pdel: "> "
    readonly property string prefix: user+pdel

    property var terminal: apx.tools.terminal

    signal focused()

    signal focusRequested() //fwd to text field


    onPrefixChanged: setCmd(getCmd())
    onFocusRequested: setFocus()

    TextInput {
        id: cmdText
        width: parent.width
        color: "white"
        font: apx.font(lineSize,true)
        wrapMode: Text.WrapAnywhere
        selectByMouse: true
        focus: true
        activeFocusOnTab: true
        readonly property int pos0: text.indexOf(pdel)+pdel.length
        readonly property int pos: cursorPosition-pos0
        text: prefix //+"</font><font color='#fff'>"

        onActiveFocusChanged: {
            if(activeFocus)consoleExec.focused()
        }

        cursorDelegate: Rectangle {
            border.width: 0
            color: "#80ffffff"
            width: height*0.4
            height: lineSize
            visible: cmdText.activeFocus
        }

        //selection
        cursorPosition: pos0
        onCursorPositionChanged: {
            if(cursorPosition<pos0)cursorPosition=pos0
        }
        onSelectionStartChanged: selectionTimer.start()
        Timer {
            id: selectionTimer
            interval: 1
            onTriggered: if(cmdText.selectionStart<cmdText.pos0)cmdText.select(cmdText.pos0,cmdText.selectionEnd)
        }
        Keys.onPressed: function(event){
            //console.log("key: "+event.key+" mod: "+event.modifiers+" text: "+event.text)
            consoleExec.focused()
            setFocus()
            if(pos<=0 && event.key===Qt.Key_Backspace){
                event.accepted=true
            }else if(event.key===Qt.Key_C && (event.modifiers&(Qt.ControlModifier|Qt.MetaModifier))){
                event.accepted=true
                text+="^C"
                reset()
            }else{ // if(event.modifiers===Qt.NoModifier){
                terminal.historyReset()
            }/*else if(event.key!==event.modifiers){
                event.accepted=true
                reset()
            }*/
        }
        Keys.onTabPressed: function(event){
            //console.log("tabE")
            event.accepted=true
            hints()
        }
        Keys.onEnterPressed: function(event){ enter(event) }
        Keys.onReturnPressed: function(event){ enter(event) }
        Keys.onUpPressed: function(event){
            event.accepted=true
            //var cpos=cursorPosition
            setCmd(terminal.historyNext(getCmd()),true)
            //cursorPosition=cpos
        }
        Keys.onDownPressed: function(event){
            event.accepted=true
            //var cpos=cursorPosition
            setCmd(terminal.historyPrev(getCmd()),true)
            //cursorPosition=cpos
        }
        function enter(event)
        {
            event.accepted=true
            exec()
        }
    }
    function setFocus()
    {
        cmdText.forceActiveFocus()
    }
    function getCmd()
    {
        return cmdText.text.slice(cmdText.pos0)
    }
    function setCmd(cmd,acmp)
    {
        cmd=prefix+cmd
        var cpos=cmdText.cursorPosition+(prefix.length-cmdText.pos0)
        if(cmdText.text===cmd)return
        cmdText.text=cmd
        cmdText.cursorPosition=acmp?cmdText.text.length:cpos
    }
    function exec()
    {
        setFocus()
        var cmd=getCmd()
        reset()
        if(cmd.length<=0)return
        terminal.exec(cmd)
    }
    function reset()
    {
        consoleExec.focused()
        terminal.enter(cmdText.text)
        setCmd("")
        terminal.historyReset()
    }

    //hints
    function hints()
    {
        var cmd=getCmd()
        var c=terminal.autocomplete(cmd)
        if(c===cmd)return
        setCmd(c,true)

    }
}
