import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

import APX.Facts 1.0

ColumnLayout {
    id: control

    property alias model: listView.model
    property alias delegate: listView.delegate
    property alias actionsModel: actionsItem.model

    property alias header: listView.header
    property alias headerItem: listView.headerItem

    property ListView listView: listView

    function factButtonTriggered(fact)
    {
        if(factMenu)
            factMenu.factButtonTriggered(fact)
    }
    property int maximumHeight: ui.window.height
                                -MenuStyle.titleSize
                                -(actionsItem.visible?actionsItem.implicitHeight+spacing:0)
                                -(listView.headerItem?listView.headerItem.implicitHeight:0)
                                -(listView.footerItem?listView.footerItem.implicitHeight:0)
                                -32

    //facts
    ListView {
        id: listView
        Layout.fillHeight: true
        Layout.fillWidth: true

        model: fact.model
        property string descr: fact.descr

        implicitHeight: Math.min(maximumHeight, Math.max(contentHeight,MenuStyle.itemSize*3)+8)
        clip: true
        focus: true
        //cacheBuffer: 0
        spacing: 0
        snapMode: ListView.SnapToItem

        //restore pos
        onVisibleChanged: {
            if(listView && visible){
                forceLayout()
                positionViewAtIndex(currentIndex,ListView.Center)
            }
        }
        headerPositioning: ListView.OverlayHeader

        //resize to contents
        onCountChanged: updateHeight()
        onHeaderItemChanged: updateHeight()

        delegate: Loader{
            asynchronous: true
            active: modelData?modelData.visible:false
            visible: active
            width: control.width
            height: active?MenuStyle.itemSize:0
            sourceComponent: Component {
                FactButton {
                    fact: modelData;
                    height: MenuStyle.itemSize
                    onTriggered: {
                        listView.currentIndex=index
                        control.factButtonTriggered(modelData)
                    }
                }
            }
            onLoaded: {
                if(index==0) item.focusRequested()
            }
        }

        readonly property int sectionSize: MenuStyle.itemSize*0.5
        section.property: "modelData.section"
        section.criteria: ViewSection.FullString
        section.delegate: Label {
            width: listView.width
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: listView.sectionSize
            color: MenuStyle.cTitleSep
            font.family: font_narrow
            text: section
        }

        /*headerPositioning: ListView.OverlayHeader
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
        }*/

        //scroll
        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            width: 6
            policy: ScrollBar.AsNeeded
        }

        function updateHeight()
        {
            return
            var h=0
            var s=[]
            for(var i=0;i<count;++i){
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
            //h+=MenuStyle.itemSize

            listView.implicitHeight=h
            //console.log("h",h,count,footerItem?footerItem.implicitHeight:-1)
        }
    }

    //actions
    RowLayout {
        id: actionsItem

        Layout.alignment: Qt.AlignRight
        Layout.bottomMargin: control.spacing

        spacing: 5
        visible: repeater.count>0

        property alias model: repeater.model
        Repeater {
            id: repeater
            model: fact.actionsModel
            delegate: Loader{
                asynchronous: true
                active: modelData && modelData.visible && ((modelData.options&Fact.ShowDisabled)?true:modelData.enabled)
                visible: active
                sourceComponent: Component {
                    FactMenuAction {
                        fact: modelData
                        showText: true
                        onTriggered: control.factButtonTriggered(modelData)
                        /*Connections {
                            target: modelData
                            onTriggered: control.factTriggered(modelData)
                        }*/
                    }
                }
            }
        }
    }
}
