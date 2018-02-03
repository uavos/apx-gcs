import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

ListView {
    id: listView
    clip: true
    Layout.preferredWidth: autoResize?itemWidth:0
    Layout.preferredHeight: contentHeight //autoResize?itemsHeight:contentHeight
    //Layout.fillHeight: !autoResize
    cacheBuffer: 0
    //property int itemsHeight: Math.max(8,fact.size+2)*itemSize
    //Behavior on itemsHeight { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
    //implicitWidth: contentItem.childrenRect.width
    implicitHeight: contentHeight //contentItem.childrenRect.height
    model: fact.model
    spacing: 0
    delegate: FactMenuListItem { fact: modelData; }
    section.property: "modelData.section"
    section.criteria: ViewSection.FullString
    section.delegate: Label {
        width: listView.width
        height: itemSize*0.5
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: height*0.9
        color: Style.cTitleSep
        font.family: font_narrow
        text: section
    }
    header: Text {
        width: listView.width
        height: visible?implicitHeight:0
        verticalAlignment: Text.AlignTop
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: descrFontSize
        font.family: font_condenced
        color: Style.cTextDisabled
        visible: fact.descr && showTitle
        text: visible?fact.descr:""
    }
    footerPositioning: ListView.PullBackFooter
    footer: RowLayout {
        id: toolsLayout
        width: listView.width
        height: toolsItem.childrenRect.height
        spacing: 0
        z: 10
        visible: false //toolsItem.children.length>0
        Flow {
            id: toolsItem
            //Layout.topMargin: itemSize*0.2
            Layout.alignment: Qt.AlignRight|Qt.AlignBottom
            Layout.maximumWidth: listView.width
            spacing: 5
            padding: 0
            topPadding: 10
            property bool actionsText: true
            Repeater {
                model: fact.actionsModel
                delegate: FactMenuAction { showText: toolsItem.actionsText }
                onItemAdded: timer.restart() //toolsLayout.visible=true
            }
            onPositioningComplete: if(width>0){
                if(height>itemSize*1.5)actionsText=false
                //timer.restart()
            }
            Timer {
                id: timer
                running: false
                repeat: false
                interval: 10
                onTriggered: {
                    toolsLayout.visible=true
                    listView.footerPositioning=ListView.OverlayFooter
                }
            }
        }
    }
    ScrollBar.vertical: ScrollBar {}

    /*Component.onCompleted: {
        heightTimer.restart()
    }
    //auto resize
    Connections {
        target: listView.contentItem
        //enabled: autoResize
        onChildrenChanged: heightTimer.restart()
    }
    Timer {
        id: heightTimer
        running: false
        repeat: false
        interval: 500
        onTriggered: {
            var sz=0
            for(var child in listView.contentItem.children) {
                var c=listView.contentItem.children[child]
                if(c.visible)sz+=c.height
                //console.log(c+" "+c.title+" "+c.height+" "+sz)
            }
            sz=Math.max(listView.itemsHeight,sz)
            //console.log(sz)
            if(sz!==listView.itemsHeight)listView.itemsHeight=sz
        }
    }*/
}
