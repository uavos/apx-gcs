import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import APX.Facts 1.0

RowLayout {
    id: control

    //prefs
    property int fontSize: 12

    //model data
    property string text
    property string subsystem
    property string source
    property int type
    property int options
    property var fact
    property int timestamp

    readonly property color color: {

        var cImportant = (source==ApxApp.FromVehicle)?"#aff":"#afa"

        switch(type){
        default:
        case ApxApp.Info: return "#aaa"
        case ApxApp.Important: return cImportant
        case ApxApp.Warning: return "#ff8"
        case ApxApp.Error: return "#f88"
        }
    }
    readonly property bool html: text.startsWith("<html>")
    readonly property bool bold: {
        if(type==ApxApp.Info)return false
        return true
    }

    //internal
    /*
    readonly property bool err: line.type===1 || line.type===2 || line.type===3
    readonly property bool app: line.category==="app"
    readonly property bool qml: line.category==="qml"

    readonly property bool appGray: app && line.text.startsWith("#")
    readonly property bool appVehicle: app && (!html) && line.text.startsWith("<")
    readonly property bool appMark: (appVehicle || line.text.startsWith("["))
    readonly property string lineText: {
        var s=line.text.trim()
        if(appGray) return s.slice(1).trim()
        if(appMark) return s.slice(s.indexOf("]")+1).trim()
        if(html) return s.slice(6).trim()
        return s
    }
    readonly property string mark: {
        var s=line.text
        if(appMark)return s.slice(s.indexOf("[")+1,s.indexOf("]")).trim()
        return ""
    }*/
    Label {
        Layout.fillWidth: true
        focus: false
        color: control.color
        font.bold: control.bold
        font.pixelSize: fontSize
        //font.family: font_fixed
        wrapMode: Text.WrapAnywhere
        text: control.text
        textFormat: html?Text.RichText:Text.AutoText
    }
    Label {
        Layout.alignment: Qt.AlignRight|Qt.AlignVCenter
        visible: text
        text: control.subsystem
        color: "#aaa"
        font.pixelSize: fontSize*0.9
        background: Rectangle {
            border.width: 0
            color: "#223"
            radius: 2
        }
    }
}
