import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

ListView {
    id: listView
    clip: true
    Layout.preferredWidth: itemWidth
    Layout.preferredHeight: itemsHeight
    cacheBuffer: 0
    property int itemsHeight: Math.max(8,fact.size+2)*itemSize
    Behavior on itemsHeight { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
    //implicitWidth: contentItem.childrenRect.width
    //implicitHeight: 800 //contentItem.childrenRect.height
    model: fact.model
    spacing: 0
    delegate: delegateC
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
    footerPositioning: ListView.OverlayFooter
    footer: Flow {
        id: toolsItem
        z: 10
        width: listView.width
        //height: childrenRect.height
        layoutDirection: Qt.RightToLeft
        spacing: 5
        padding: 0
        visible: children.length>0
    }
    Component.onCompleted: {
        //create actions
        for (var i=0; i < fact.size; i++){
            var f=fact.childFact(i)
            if(!isActionFact(f))continue;
            var c=actionC.createObject(footerItem,{"fact": f});
        }
    }
    ScrollBar.vertical: ScrollBar {}
    Component {
        id: delegateC
        FactMenuListItem { fact: modelData; }
    }
    Component {
        id: actionC
        FactMenuAction { }
    }
    Connections {
        target: listView.contentItem
        enabled: autoResize
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

    }
}
