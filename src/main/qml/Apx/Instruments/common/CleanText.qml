import QtQuick 2.2
import QtQuick.Layouts 1.12
import "."

Item {
    id: control
    property var fact

    enum Type {
        Normal,
        Clean,
        White,
        Green,
        Yellow,
        Red,
        Blue,
        Black
    }

    property int type: CleanText.Normal

    property string text: fact.text
    property string prefix
    property string toolTip: fact.title+": "+fact.text

    property string displayText: (control.prefix ? control.prefix + " " + control.text : control.text).toUpperCase()

    property string font: font_narrow
    property int fontSize: Math.max(4, fix_font?(height*1.2):height)

    property bool show: true
    property bool frame: false

    property color bgColor: {
        switch(type){
            default: return "#80000000"
            case CleanText.Clean: return "transparent"
            case CleanText.White: return "#ddd"
            case CleanText.Green: return "green"
            case CleanText.Yellow: return "yellow"
            case CleanText.Red: return "red"
            case CleanText.Blue: return "blue"
            case CleanText.Black: return "black"
        }
    }
    property color textColor: {
        if(frame)
            return "white"
        switch(type){
            default: return "white"
            case CleanText.Yellow:
            case CleanText.White:
                return "black"
        }
    }

    readonly property bool fix_font: font === font_narrow

    Component.onCompleted: {
        if(!fact)
            console.info(control.text, parent, parent.parent)
    }

    property bool hide_bg: hide_fg
    property bool hide_fg: !(show || ui.test)

    Rectangle {
        anchors.fill: parent
        visible: !frame
        color: bgColor
        opacity: hide_bg?0:1
        radius: 2
    }

    implicitWidth: _body.implicitWidth + 4
    implicitHeight: Math.max(8, textItem.implicitHeight)

    property alias prefixItems: _prefixItems.children
    RowLayout {
        id: _body
        anchors.fill: parent
        opacity: hide_fg?0:1
        spacing: 0
        RowLayout {
            id: _prefixItems
            Layout.fillWidth: false
            Layout.fillHeight: true
            spacing: 0
        }
        Text {
            id: textItem
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: fix_font?font.pixelSize*0.05+1:0
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: control.displayText
            font.pixelSize: fontSize
            font.family: control.font
            font.bold: (control.font !== font_narrow)
            color: textColor
        }
    }
    Rectangle {
        anchors.fill: _body
        anchors.leftMargin: _prefixItems.width
        visible: frame
        color: "transparent"
        radius: 2
        border.width: 1
        border.color: bgColor
    }

    ToolTipArea {
        text: control.toolTip
    }
}

