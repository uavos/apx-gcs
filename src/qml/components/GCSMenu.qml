import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQml.Models 2.2
import "."

Item {
    id: root
    //focus: true

    signal closed()
    signal opened()


    function openMenuField(field)
    {
        openPage({"fields": field.fields,"title": field.title})
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

    property string title

    property int itemSize: 32

    property int btnSize: itemSize*0.9

    property color colorBg: "#C0000000"
    property color colorBgAlt: "transparent" //"#15f0f0f0"
    property color colorBgHover: "#30f0f0f0"
    property color colorBgPress: "#5530FF60"
    property color colorTitleSep: "#5c8fff"
    property color colorSep: "#667"

    property bool showTitle: title || showBtnBack || showBtnClose

    property bool showBtnBack: stackView.depth>1 || parentStack
    property bool showBtnClose: closeable

    property int titleSize: closeable?itemSize*1.2:itemSize
    property int itemWidth: stackView.width-stackView.leftPadding*2

    property ObjectModel fields

    signal pageDeleted()

    signal updateListView()

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 4
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
            property ObjectModel fields: root.fields //root.contents  //: listView.model
            property string title: root.title //pageTitleLdr.title
            property string page
            //Component.onDestruction: console.log("page delete: "+title)
            StackView.onRemoved: { destroy(); root.pageDeleted(); }

            Item {
                Layout.fillWidth: true
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
                    icon: "navigation_previous_item.png"
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
                clip: true

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
                        model: menuPage.fields
                        spacing: 0
                        //cacheBuffer: 0
                        //focus: true
                        //delegate: GCSMenuField { }
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
            }
        }
    }


}

