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

        var cImportant = (source==AppNotify.FromVehicle)?"#aff":"#afa"
        var cInfo = (source==AppNotify.FromInput)?"#ccc":"#aaa"

        switch(type){
        default:
        case AppNotify.Info: return cInfo
        case AppNotify.Important: return cImportant
        case AppNotify.Warning: return "#ff8"
        case AppNotify.Error: return "#f88"
        }
    }
    readonly property bool html: text.startsWith("<html>")
    readonly property bool bold: {
        if(source==AppNotify.FromInput) return true
        if(type==AppNotify.Info) return false
        return true
    }

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
        visible: text //&& control.x==0
        text: control.subsystem
        color: "#aaa"
        font.family: font_condenced
        font.pixelSize: fontSize*0.9
        background: Rectangle {
            border.width: 0
            color: "#223"
            radius: 2
        }
    }
}
