import QtQuick 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

Button {
    id: control

    property string iconName
    property color color: Material.background

    property bool showText: false
    property int iconSize: 24

    signal menuRequested()
    signal triggered()
    signal activated()

    text: "control"

    //internal
    property bool bText: iconName?showText && text:true

    padding: iconName?3:5
    leftPadding: padding+1
    rightPadding: padding+1
    topPadding: padding
    bottomPadding: padding
    spacing: 3

    background.y: 0
    background.width: width
    background.height: height-1

    Material.background: color

    implicitHeight: visible?contentItem.implicitHeight+topPadding+bottomPadding:0
    implicitWidth: bText?Math.max(implicitHeight,contentItem.implicitWidth+leftPadding+rightPadding):implicitHeight

    Component.onCompleted: {
        //if(color)Material.background=Qt.binding(function(){return color})
    }

    contentItem: RowLayout {
        spacing: control.spacing
        //icon
        Label {
            visible: control.iconName
            Layout.fillHeight: true
            Layout.fillWidth: !bText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: bText?Text.AlignLeft:Text.AlignHCenter
            font.family: "Material Design Icons"
            font.pointSize: control.iconSize
            text: visible?materialIconChar[iconName]:""
        }
        //text
        Label {
            visible: bText
            Layout.fillHeight: true
            Layout.minimumWidth: font.pixelSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font: control.font
            text: control.text
        }
    }

    property bool blockPressAndHold: false

    onClicked: {
        blockPressAndHold=true
        triggered()
        blockPressAndHold=false
    }
    onPressAndHold: {
        if(blockPressAndHold)return
        menuRequested()
    }
    onCheckedChanged: if(checked)activated()
}
