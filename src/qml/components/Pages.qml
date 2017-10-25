import QtQuick 2.2
import QtQuick.Controls 1.1

Item {
    id: pagesItem
    property string title
    property ListModel listModel
    property bool saveState: false

    property int itemHeight: Math.max(20,parent.height*0.08)
    property int fontSize: itemHeight*0.8

    property string currentTitle: title


    function back()
    {
        stackView.pop()
        currentTitle=title;
        m.settings.setValue(pagesItem.title+"Page","");
    }

    function go(title,page)
    {
        currentTitle=title;
        stackView.push(Qt.resolvedUrl("../"+page))
        if(saveState) m.settings.setValue(pagesItem.title+"Page",title);
    }


    Rectangle {
        color: "#181818"
        anchors.fill: parent
    }

    Rectangle {
        id: toolBar
        color: "black"
        width: parent.width
        height: itemHeight
        z: 1

        Rectangle {
            id: backButton
            width: opacity ? fontSize : 0
            anchors.left: parent.left
            anchors.leftMargin: 5
            opacity: stackView.depth > 1 ? 1 : 0
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true
            height: itemHeight
            radius: 4
            color: (backmouse.pressed || backmouse.containsMouse) ? "#222" : "transparent"
            Behavior on opacity { NumberAnimation{} }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: "navigation_previous_item.png"
                sourceSize.height: fontSize
            }
            MouseArea {
                id: backmouse
                anchors.fill: parent
                anchors.margins: -5
                hoverEnabled: true
                onClicked: back()
            }
        }

        Text {
            font.pixelSize: fontSize
            Behavior on x { NumberAnimation{ easing.type: Easing.OutCubic} }
            x: backButton.x + backButton.width + 20
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            text: currentTitle
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            color: "#5c8fff"
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.topMargin: toolBar.height
        property string currentTitle: title

        //load last page
        Component.onCompleted: {
            if(saveState){
                var s=settings.value(title+"Page");
                if(s){
                    for(var i = 0; i < listModel.count; i++) {
                        var e=listModel.get(i);
                        if(e.title === s) {
                            stackView.push(Qt.resolvedUrl("../"+e.page));
                            break;
                        }
                    }
                }
            }
        }


        // Implements back key navigation
        focus: true
        Keys.onReleased: if (event.key === Qt.Key_Back && stackView.depth > 1) {
                             back();
                             event.accepted = true;
                         }

        initialItem: Item {
            width: parent.width
            height: parent.height
            ListView {
                model: listModel
                anchors.fill: parent
                delegate: stackItemDelegate
            }
        }
    }

    Component {
        id: stackItemDelegate
        Item {
            id: root
            width: parent.width
            height: itemHeight

            signal clicked

            onClicked: go(title,page)

            Rectangle {
                anchors.fill: parent
                color: "#11ffffff"
                visible: mouse.pressed || mouse.containsMouse
            }

            Text {
                id: textitem
                color: "white"
                font.pixelSize: fontSize
                text: title
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 30
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 15
                height: 1
                color: "#424246"
            }

            Image {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                sourceSize.height: fontSize
                source: "navigation_next_item.png"
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.clicked()
            }
        }
    }
}
