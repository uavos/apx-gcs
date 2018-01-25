import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0
import "../components"
import "."

ColumnLayout {
    id: menuPage

    //Fact bindings
    property var fact

    //implicitHeight: 500 //childrenRect.height
    //implicitWidth: 300
    //Component.onDestruction: console.log("page delete: "+fact.title)
    StackView.onRemoved: destroy()

    Connections {
        target: fact
        onRemoved: {
            fact=dummyFactC.createObject(menuPage);
            destroy()
        }
    }
    Component {
        id: dummyFactC
        FactMenuElement { }
    }

    property int padding: 4
    clip: true

    Component.onCompleted: {
        if(showTitle){
            pageTitleC.createObject(this,{
                "clip": true,
                "Layout.fillWidth": true,
                "Layout.leftMargin": padding,
                "Layout.rightMargin": padding,
                "Layout.topMargin": padding
            });
        }
        var opts={
            "clip": true,
            "Layout.fillWidth": true,
            "Layout.fillHeight": true,
            "Layout.leftMargin": padding,
            "Layout.rightMargin": padding,
            "Layout.bottomMargin": padding
        }
        if(fact.qmlPage){
            var c=Qt.createComponent(fact.qmlPage,Component.Asynchronous,menuBody);
            c.statusChanged.connect( function(status) {
                if (status === Component.Ready) {
                    c.createObject(menuBody,opts);
                }
            })
            c.statusChanged(c.status);
        }else{
            listViewC.createObject(this,opts);
        }
    }


    //Page Title
    Component {
        id: pageTitleC
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
    }



    //Page Body
    Component {
        id: listViewC
        ListView {
            id: listView
            clip: true
            Layout.preferredWidth: itemWidth
            Layout.preferredHeight: itemsHeight//+headerItem.height//footerItem.height
            cacheBuffer: 0
            property int itemsHeightMin: 8*itemSize
            property int itemsHeight: itemsHeightMin
            Behavior on itemsHeight { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
            //implicitWidth: contentItem.childrenRect.width
            //implicitHeight: 800 //contentItem.childrenRect.height
            property bool bEnumItemFact: fact.size===0 && fact.enumStrings.length>0
            model: bEnumItemFact?fact.enumStrings:fact.model
            spacing: 0
            delegate: delegateC //FactMenuItem { parentFact: listView.fact; bEnumItemFact: listView.bEnumItemFact; }
            section.property: "modelData.section"
            section.criteria: ViewSection.FullString
            section.delegate: Label {
                width: listView.width
                height: itemSize*0.6
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
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: itemSize*0.4
                font.family: font_condenced
                color: "#aaa"
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
                FactMenuListItem { fact: modelData; } //parentFact: listView.fact; bEnumItemFact: listView.bEnumItemFact; }
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
                interval: 100
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
    }
}
