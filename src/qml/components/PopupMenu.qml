import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import "."

Popup {
    id: root
    //modal: true
    focus: true
    parent: window

    //contentWidth: Math.min(window.width, window.height) / 3 * 2
    //contentHeight: Math.min(window.height, contents.length*menu.height)
    x: (window.width - width) / 2
    y: (window.height - height) / 2
    contentHeight: 10*itemSize+itemSize*1.2
    contentWidth: itemSize*10 //cWidth

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 0.9 }
    }

    //padding: 8

    onClosed: stackView.pop(null);

    function openPage(page)
    {
        stackView.push(stackViewComponent.createObject(),{"model": page.contents,"title": page.title})
        open()
    }

    //property int cWidth

    property string title
    property int itemSize: 32

    property int btnSize: itemSize*0.8

    property color bgColor: "#C0000000"
    property color bgColorHover: "#30000000"
    //width: (itemSize)*10 //mapProvider.width/4
    //height: (itemSize)*contents.length+menuHeader.height+radius

    //property int radius: itemSize/3

    default property alias contents: addItem.children
    Item { id: addItem }

    function appendItem(object)
    {
        object.parent=addItem
    }
    function removeItem(object)
    {
        delete object
    }

    contentItem: Item {
        anchors.fill: parent
        anchors.margins: 4
        //anchors.margins: root.radius/2
        /*Item {
            id: menuHeader
            width: parent.width
            height: root.itemSize*1.2
            clip: true
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 2
                color: "#5c8fff"
            }
            Text {
                id: titleText
                anchors.top: parent.top
                anchors.left: btnBack.visible?btnBack.right:parent.left
                anchors.leftMargin: 8
                //anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: root.itemSize //*1.1
                font.family: font_narrow
                color: "white"
                visible: text!=""
                text: root.title
            }
            FastBlur {
                anchors.fill: titleText
                //anchors.centerIn: titleText
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
                visible: stackView.depth>1
                anchors.left: parent.left
                anchors.leftMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                icon: "navigation_previous_item.png"
                height: itemSize
                width: height
                onClicked: stackView.pop();
            }

        }
        Item {
            id: menuBody
            anchors.top: menuHeader.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            clip: true
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
                initialItem: stackViewComponent.createObject()
            }
        }

        Component {
            id: stackViewComponent
            ListView {
                model: contents
                delegate: menuElement
                StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
            }
        }*/

        Item {
            width: parent.width
            height: root.itemSize*1.2
            Btn {
                //anchors.top: parent.top
                //anchors.topMargin: 3
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.verticalCenter: parent.verticalCenter
                height: itemSize
                width: height
                text: qsTr("X")
                colorBG: "#808f478f"
                //highlighted: true
                //flat: true
                //hoverEnabled: true
                onClicked: root.close()
                //Material.foreground: "#bbb"

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
            initialItem: stackViewComponent.createObject()
        }


        Component {
            id: stackViewComponent
            Item {
                property alias model: listView.model
                property alias title: titleText.text
                Item {
                    id: menuHeader
                    width: parent.width
                    height: root.itemSize*1.2
                    clip: true
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 2
                        color: "#5c8fff"
                    }
                    Text {
                        id: titleText
                        anchors.top: parent.top
                        anchors.left: btnBack.visible?btnBack.right:parent.left
                        anchors.leftMargin: 8
                        //anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: root.itemSize //*1.1
                        font.family: font_narrow
                        color: "white"
                        visible: text!=""
                        text: root.title
                    }
                    FastBlur {
                        anchors.fill: titleText
                        //anchors.centerIn: titleText
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
                        visible: stackView.depth>1
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        icon: "navigation_previous_item.png"
                        height: itemSize
                        width: height
                        onClicked: stackView.pop();
                    }

                }
                Item {
                    id: menuBody
                    anchors.top: menuHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    clip: true
                    ListView {
                        id: listView
                        anchors.fill: parent
                        model: contents
                        delegate: menuElement
                        StackView.onRemoved: destroy() // Will be destroyed sometime after this call.
                    }
                }
            }
        }




        Component {
            id: menuElement
            Rectangle {
                //width: root.width-root.radius
                width: parent.width
                height: modelData.title?root.itemSize:2
                border.width: 0
                color: mouseArea.containsMouse?root.bgColorHover:(index&1?"#15000000":"transparent")
                //Behavior on color { ColorAnimation {duration: 100; } }

                //Component.onCompleted: root.cWidth=Math.max(root.cWidth, menuItemText.width+8*2)

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    //propagateComposedEvents: true
                    //cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        //console.log("click menu");
                        root.focus=true;
                        modelData.clicked()
                        if(modelData.contents.length>0){
                            stackView.push(stackViewComponent.createObject(),{"model": modelData.contents,"title": modelData.title})
                        }else if(modelData.checkable){
                            //modelData.checked=!modelData.checked
                            //menuItemSwitch.toggle()
                        }else root.close();
                    }
                }
                Item {
                    id: menuField
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    clip: true
                    Text {
                        id: menuItemText
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: root.itemSize*0.6
                        color: "#fff"
                        font.family: font_condenced
                        text: modelData.title
                    }
                    Rectangle {
                        id: menuItemBG
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 1
                        height: 1
                        color: "#667"
                    }
                    Image {
                        visible: modelData.contents.length
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.height: root.btnSize
                        source: "navigation_next_item.png"
                    }
                    /*CheckBox {
                        visible: modelData.checkable
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        checked: modelData.checked
                        onClicked: {
                            modelData.clicked();
                            modelData.checked=checked
                        }
                    }*/
                    Switch {
                        id: menuItemSwitch
                        visible: modelData.checkable
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        checked: modelData.checked
                        onClicked: {
                            modelData.clicked();
                            //modelData.checked=checked
                        }
                    }

                }
            }
        }
    }

}

