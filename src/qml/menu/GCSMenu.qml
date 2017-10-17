import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0
import "../components"
import "."

Item {
    id: root
    focus: true

    signal closed()
    signal opened()


    function openMenuField(field)
    {
        openPage({"fields": field.fields,"title": field.title})
    }

    function openFact(factItem)
    {
        openPage({"fact": factItem})
    }

    function openPage(opts)
    {
        stackView.push(pageDelegate.createObject(stackView,opts))
        opened()
    }

    function pushUrl(url,opts)
    {
        stackView.push(Qt.resolvedUrl("../"+url),opts?opts:{"parentStack": stackView})
    }

    function close()
    {
        stackView.pop(null);
        closed();
    }

    function back()
    {
        if(stackView.depth>1)stackView.pop();
        else if(parentStack)parentStack.pop();
    }


    property StackView parentStack

    property bool closeable: false

    property bool effects: true

    property string title: fact?fact.title:""

    property int itemSize: 32

    property int btnSize: itemSize*0.9

    property color colorBg: "#C0000000"
    property color colorBgField: "#80303030"
    property color colorBgHover: "#40f0f0f0"
    property color colorBgPress: "#5530FF60"
    property color colorTitleSep: "#5c8fff"
    property color colorSep: "#667"
    property color colorValueText:      "#30FF60"
    property color colorValueTextEdit:  "#FFFF60"
    property color colorActionRemove:   "#a55"

    property url iconPrev: Qt.resolvedUrl("navigation_previous_item.png")
    property url iconNext: Qt.resolvedUrl("../menu/navigation_next_item.png")

    property bool showTitle: title || showBtnBack || showBtnClose

    property bool showBtnBack: stackView.depth>1 || parentStack
    property bool showBtnClose: closeable

    property int titleSize: closeable?itemSize*1.2:itemSize
    property int itemWidth: stackView.width-stackView.leftPadding*2

    property GCSMenuModel fields
    property Fact fact

    signal pageDeleted()

    signal updateListView()

    StackView {
        id: stackView
        anchors.fill: parent
        //anchors.margins: 4
        clip: true
        // Implements back key navigation
        focus: true
        /*Keys.onReleased: if (event.key === Qt.Key_Back && stackView.depth > 1) {
                             back();
                         }*/
        initialItem: pageDelegate.createObject(stackView)
    }

    Btn {
        id: btnClose
        visible: showBtnClose
        anchors.right: parent.right
        anchors.rightMargin: 4+3
        y: titleSize/2-btnSize/2+2
        height: btnSize
        width: height
        text: "X"
        colorBG: "#80cf478f"
        onClicked: root.close()
        effects: root.effects
    }


    Component {
        id: pageDelegate
        ColumnLayout {
            id: menuPage

            property GCSMenuModel fields: root.fields //root.contents  //: listView.model
            property Fact fact: root.fact
            property string title: fact?fact.title:root.title //pageTitleLdr.title
            property string page
            //Component.onDestruction: console.log("page delete: "+title)
            StackView.onRemoved: { destroy(); root.pageDeleted(); }

            property int padding: 4
            Item {
                Layout.fillWidth: true
                Layout.topMargin: padding
                Layout.leftMargin: padding
                Layout.rightMargin: padding
                implicitHeight: titleSize //showTitle?titleSize:2
                clip: true
                visible: showTitle
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 2
                    color: colorTitleSep
                }
                Text {
                    id: titleText
                    anchors.top: parent.top
                    anchors.left: showBtnBack?btnBack.right:parent.left
                    anchors.leftMargin: 8
                    font.pixelSize: root.titleSize*0.8 //*1.1
                    font.family: font_narrow
                    color: "white"
                    visible: text!=""
                    text: menuPage.title
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
                    icon: iconPrev
                    height: btnSize
                    width: height
                    onClicked: back()
                    effects: root.effects
                }
            }

            Item {
                id: menuBody
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.leftMargin: padding
                Layout.rightMargin: padding
                clip: true
                //anchors.margins: 4

                Component.onCompleted: {
                    if(page){
                        var c=Qt.createComponent(page,Component.Asynchronous,menuBody);
                        c.statusChanged.connect( function(status) {
                            if (status === Component.Ready) {
                                c.createObject(menuBody,{"anchors.fill": menuBody});
                            }
                        })
                        c.statusChanged(c.status);
                    }else{
                        listViewC.createObject(menuBody,{"anchors.fill": menuBody});
                    }
                }
                Component {
                    id: listViewC
                    ListView {
                        id: listView
                        model: menuPage.fact?menuPage.fact:menuPage.fields
                        spacing: 0
                        //cacheBuffer: 0
                        //focus: true
                        delegate: GCSMenuField { fact: modelData }
                        section.property: "modelData.section"
                        section.criteria: ViewSection.FullString
                        section.delegate: GCSMenuField { title: section; separator: true; }
                        /*Connections {
                            target: root
                            onUpdateListView: {
                                //listView.model="undefined"
                                //listView.model=menuPage.model
                                //console.log("updateList")
                                //listView.forceLayout();
                            }
                        }*/
                    }
                }
                Component {
                    id: listViewDelegate
                    GCSMenuField {
                    }
                }

            }
        }
    }


}

