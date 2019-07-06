import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

RowLayout {
    property var line
    property int fontSize: 12

    readonly property bool html: line.text.startsWith("<html>")
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
    }
    Label {
        Layout.fillWidth: true
        focus: false
        color: err?"#f88":qml?"#ccd":app?(appGray?"#aaa":appVehicle?"#aff":"#afa"):"white"
        font.bold: !qml
        font.pixelSize: fontSize
        //font.family: font_fixed
        wrapMode: Text.WrapAnywhere
        text: lineText
        textFormat: html?Text.RichText:Text.AutoText
    }
    Label {
        Layout.alignment: Qt.AlignRight|Qt.AlignVCenter
        visible: mark.length>0
        text: mark
        color: "#aaa"
        font.pixelSize: fontSize*0.9
        background: Rectangle {
            border.width: 0
            color: "#223"
            radius: 2
        }
    }
}
