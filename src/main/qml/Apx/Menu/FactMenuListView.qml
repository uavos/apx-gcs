import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3


ListView {
    id: listView

    model: fact.model
    property string descr: fact.descr

    implicitHeight: MenuStyle.itemSize //contentHeight
    clip: true
    focus: true
    //cacheBuffer: 0
    spacing: 0
    snapMode: ListView.SnapToItem
    readonly property int sectionSize: MenuStyle.itemSize*0.5
    section.property: "modelData.section"
    section.criteria: ViewSection.FullString
    section.delegate: Label {
        width: listView.width
        height: sectionSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Math.max(8,height*0.9)
        color: MenuStyle.cTitleSep
        font.family: font_narrow
        text: section
    }

    headerPositioning: ListView.OverlayHeader
    header: Text {
        width: listView.width
        height: visible?implicitHeight:0
        verticalAlignment: Text.AlignTop
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: MenuStyle.itemSize*0.33
        font.family: font_condenced
        color: MenuStyle.cTextDisabled
        visible: listView.descr && showTitle
        text: visible?listView.descr:""
    }

    //scroll
    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        width: 6
        policy: ScrollBar.AsNeeded
    }

    //restore pos
    onVisibleChanged: {
        if(listView && visible){
            forceLayout()
            positionViewAtIndex(currentIndex,ListView.Center)
        }
    }

    //resize to contents
    property int itemsCount: count
    onItemsCountChanged: updateHeight()
    onHeaderItemChanged: updateHeight()

    function updateHeight()
    {
        if(!listView)return
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
        //if(footerItem)h+=footerItem.height
        if(s[0]==="")s.shift()
        h+=sectionSize*s.length
        h+=MenuStyle.itemSize

        listView.implicitHeight=h
        //console.log("h",h,count,footerItem?footerItem.implicitHeight:-1)
    }
}
