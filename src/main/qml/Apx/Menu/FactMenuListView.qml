import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3


ListView {
    id: control
    //implicitWidth: MenuStyle.itemWidth
    implicitHeight: MenuStyle.itemSize //contentHeight
    clip: true
    focus: true
    //cacheBuffer: 0
    model: fact.model
    spacing: 0
    snapMode: ListView.SnapToItem
    readonly property int sectionSize: MenuStyle.itemSize*0.5
    section.property: "modelData.section"
    section.criteria: ViewSection.FullString
    section.delegate: Label {
        width: control.width
        height: sectionSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Math.max(8,height*0.9)
        color: MenuStyle.cTitleSep
        font.family: font_narrow
        text: section
    }
    header: Text {
        width: control.width
        height: visible?implicitHeight:0
        verticalAlignment: Text.AlignTop
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: MenuStyle.itemSize*0.33
        font.family: font_condenced
        color: MenuStyle.cTextDisabled
        visible: fact.descr && showTitle
        text: visible?fact.descr:""
    }
    footerPositioning: ListView.OverlayFooter
    Item {
        id: actionsContainer
    }
    property alias contentsActions: actionsContainer.children
    footer: RowLayout {
        id: toolsLayout
        z: 10
        width: control.width
        //height: toolsItem.height //childrenRect.height
        spacing: 5
        visible: false
        property bool actionsText: true
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            Layout.topMargin: toolsLayout.visible?10:0
            Repeater {
                model: fact.actionsModel
                delegate: FactMenuAction { factAction: modelData; showText: toolsLayout.actionsText }
                onItemAdded: timer.restart()
            }
            Timer {
                id: timer
                running: false
                repeat: false
                interval: 10
                onTriggered: {
                    for(var i=0;i<contentsActions.length;++i){
                        var c=contentsActions[i]
                        c.parent=toolsLayout
                    }
                    toolsLayout.visible=true
                    updateHeight()
                }
            }
            readonly property var contentsActions: actionsContainer.children
            onContentsActionsChanged: if(contentsActions.length>0)timer.restart()
        }
    }
    onHeightChanged: {
        footerPositioning=ListView.PullBackFooter
        footerPositioning=ListView.OverlayFooter
    }

    //scroll
    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        width: 6
        policy: ScrollBar.AsNeeded //atTop?ScrollBar.AsNeeded:ScrollBar.AlwaysOn
        //property bool atTop: (control.originY-control.contentY)==0
        //onAtTopChanged: console.log(atTop)
    }

    //restore pos
    onVisibleChanged: {
        if(control && visible){
            forceLayout()
            positionViewAtIndex(currentIndex,ListView.Center)
        }
    }

    //resize to contents
    property int itemsCount: count
    onItemsCountChanged: updateHeight()
    onHeaderItemChanged: updateHeight()
    onFooterItemChanged: updateHeight()

    function updateHeight()
    {
        if(!control)return
        var h=0
        var s=[]
        for(var i=0;i<itemsCount;++i){
            var f=model[i]
            if(!f){
                if(typeof(model.get)=='function'){
                    f=model.get(i)
                }
            }
            if(!f)continue;
            if(typeof(f.visible)!=='undefined'){
                f.visibleChanged.disconnect(updateHeight)
                f.visibleChanged.connect(updateHeight)
                if(!f.visible) continue
            }
            h+=MenuStyle.itemSize+spacing
            if(s.indexOf(f.section)>=0)continue
            s.push(f.section)
        }
        if(headerItem)h+=headerItem.height
        if(footerItem)h+=footerItem.height
        if(s[0]==="")s.shift()
        h+=sectionSize*s.length
        h+=MenuStyle.itemSize

        control.implicitHeight=h
        //console.log("h",h,count,footerItem?footerItem.implicitHeight:-1)
    }
}
