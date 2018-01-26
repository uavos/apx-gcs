import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "../components"
import "."

Item {
    //property var fact
    height: titleSize
    clip: true
    Text {
        id: titleText
        anchors.top: parent.top
        anchors.left: showBtnBack?btnBack.right:parent.left
        anchors.leftMargin: 8
        font.pixelSize: parent.height*0.8
        font.family: font_narrow
        color: "white"
        visible: text!=""
        text: fact.title
    }
    FastBlur {
        anchors.fill: titleText
        transparentBorder: true
        source: titleText
        radius: titleText.height/2
        visible: effects && titleText.visible
    }
    Btn {
        id: btnBack
        visible: showBtnBack
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -2
        iconName: "chevron-left"
        height: parent.height*0.8
        width: height
        onClicked: back()
        effects: root.effects
    }
    ProgressBar {
        anchors.right: parent.right
        anchors.left: titleText.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        //anchors.leftMargin: itemSize
        //anchors.verticalCenter: parent.verticalCenter
        property int v: fact.progress
        visible: v>0
        value: v/100
        indeterminate: v<0
        //opacity: 0.7
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 2
        color: Style.cTitleSep
    }
}
