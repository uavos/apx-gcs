import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import "."

Item {
    id: root
    focus: true

    signal closed()
    signal opened()


    function openPage(page)
    {
        stackView.push(pageDelegate.createObject(),{"model": page.contents,"title": page.title})
        opened()
    }

    function close()
    {
        stackView.pop(null);
        closed();
    }

    property StackView parentStack

    property bool closeable: false

    property string title

    property int itemSize: 32

    property int btnSize: itemSize*0.9
    property int btnSizeNext: itemSize*0.8

    property color colorBg: "#C0000000"
    property color colorBgAlt: "transparent" //"#15f0f0f0"
    property color colorBgHover: "#304040f0"
    property color colorBgPress: "#5530FF60"
    property color colorTitleSep: "#5c8fff"
    property color colorSep: "#667"

    property bool showTitle: title || showBtnBack || btnClose.visible

    property bool showBtnBack: stackView.depth>1 || parentStack
    property bool showBtnClose: closeable

    property int titleSize: closeable?itemSize*1.2:itemSize


    property GCSMenuItem menu: GCSMenuItem {}

    property ListModel model: ListModel {}

    property var contents: root.menu.contents
    //property alias contents: root.model


    Item { //content
        anchors.fill: parent
        anchors.margins: 4
        Item {
            width: parent.width
            height: showTitle?titleSize:0
            visible: showBtnClose
            Btn {
                id: btnClose
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -2
                height: btnSize
                width: height
                text: qsTr("X")
                colorBG: "#808f478f"
                onClicked: root.close()
            }
        }


        StackView {
            id: stackView
            anchors.fill: parent
            clip: true
            // Implements back key navigation
            focus: true
            Keys.onReleased: if (event.key === Qt.Key_Back && stackView.depth > 1) {
                                 stackView.pop()
                                 event.accepted = true;
                             }
            initialItem: pageDelegate.createObject()
        }


    }

    Component {
        id: pageDelegate
        ColumnLayout {
            id: menuPage
            property alias model: listView.model
            property alias title: pageTitleLdr.title
            property string page
            Loader {
                id: pageTitleLdr
                Layout.fillWidth: true
                active: showTitle
                sourceComponent: pageTitle
                visible: active
                property string title: root.title
            }
            Item {
                id: menuBody
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                ListView {
                    id: listView
                    anchors.fill: parent
                    visible: !page
                    model: contents
                    spacing: 1
                    delegate: Component {
                        Loader {
                            id: loader
                            source: "GCSMenuField.qml"
                            //asynchronous: true
                            //sourceComponent: menuField
                            //property QtObject data: model
                            /*Binding {
                              target: loader.item
                              property: "model"
                              value: model
                              when: loader.status == Loader.Ready
                            }*/
                        }
                    }
                    StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
                }
                Loader {
                    id: pageView
                    anchors.fill: parent
                    visible: page
                    source: page
                    active: source
                    StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
                    asynchronous: true //mandala.smooth
                }
            }
        }
    }


    /*Component {
        id: pageBodyDelegate
        Item {
            id: menuBody
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            ListView {
                id: listView
                anchors.fill: parent
                visible: !page
                model: contents
                delegate: menuField
                StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
            }
            Loader {
                id: pageView
                anchors.fill: parent
                visible: page
                source: page
                active: source
                StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
                asynchronous: true //mandala.smooth
            }
        }
    }*/




    /*Component {
        id: menuField
        Rectangle {
            property QtObject modelData
            width: parent.width
            height: modelData.height?modelData.height:modelData.title?root.itemSize:2 //empty is separator
            border.width: 0
            color: mouseArea.containsMouse?(mouseArea.pressed?colorBgPress:root.colorBgHover):(modelData.index&1?colorBgAlt:"transparent")
            //Behavior on color { ColorAnimation {duration: 100; } }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                //propagateComposedEvents: true
                //cursorShape: Qt.PointingHandCursor
                onClicked: {
                    //console.log("click menu");
                    menuFieldBusy.start();
                    root.focus=true;
                    modelData.clicked()
                    if(modelData.contents.length>0){
                        openPage(modelData)
                    }else if(modelData.checkable){
                        //modelData.checked=!modelData.checked
                        //menuItemSwitch.toggle()
                    }else if(modelData.subMenu){
                        stackView.push(Qt.resolvedUrl("../"+modelData.subMenu),{"parentStack": stackView})
                    }else if(modelData.page){
                        stackView.push(pageDelegate.createObject(),{"page": Qt.resolvedUrl("../"+modelData.page),"title": modelData.title})
                    }else if(closeable)root.close();
                }
            }
            Rectangle {
                id: menuItemSeparator
                anchors.left: menuFieldBody.left
                anchors.right: menuFieldBody.right
                anchors.margins: 1
                height: 1
                color: modelData.index>0?"#667":"transparent"
            }
            RowLayout {
                id: menuFieldBody
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                anchors.topMargin: 0
                anchors.bottomMargin: 0
                spacing: 0
                clip: true
                Text { //title
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: root.itemSize*0.6
                    color: "#fff"
                    font.family: font_condenced
                    text: modelData.title
                    clip: true
                }
                BusyIndicator {
                    id: menuFieldBusy
                    running: (modelData.busy!=="undefined")?modelData.busy:false
                    visible: running
                    Layout.fillHeight: true
                    implicitWidth: height
                    function start()
                    {
                        menuFieldBusy.running=true;
                        menuFieldBusyTimer.start();
                    }
                    Timer {
                        id: menuFieldBusyTimer
                        interval: 500; running: false; repeat: false
                        onTriggered: menuFieldBusy.running=(modelData.busy!=="undefined")?Qt.binding(function() { return modelData.busy }):false
                    }
                }
                //optional field editors
                Loader { //submenu
                    active: modelData.contents.length || modelData.subMenu || modelData.page
                    visible: active
                    Layout.fillHeight: true
                    Layout.margins: 4
                    sourceComponent: Image {
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.height: root.btnSizeNext
                        source: "navigation_next_item.png"
                    }
                }
                Loader { //switch
                    active: modelData.checkable
                    visible: active
                    Layout.fillHeight: true
                    sourceComponent: Switch {
                        anchors.verticalCenter: parent.verticalCenter
                        checked: modelData.checked
                        onClicked: {
                            menuFieldBusy.start();
                            modelData.clicked()
                        }
                    }
                }
            }
        }
    }*/


    Component {
        id: pageTitle
        Item {
            //property alias title: titleText.text
            //id: pageHeader
            //width: menuPage.width
            //Layout.fillWidth: true
            implicitHeight: titleSize //showTitle?titleSize:2
            clip: true
            //visible: showTitle
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
                text: title
            }
            FastBlur {
                anchors.fill: titleText
                transparentBorder: true
                source: titleText
                radius: titleText.height/2
                visible: titleText.visible
            }
            Text {
                id: infoText
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                font.pixelSize: root.itemSize*0.8
                font.family: font_narrow
                color: "gray"
                visible: text!=""
                //text: modelData.value
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
                onClicked: {
                    if(stackView.depth>1)stackView.pop();
                    else if(parentStack)parentStack.pop();
                }
            }
        }
    }



}

