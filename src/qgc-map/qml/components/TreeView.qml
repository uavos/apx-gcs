import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

Item {
    id: root
    width: 600 // change to parent.width
    height: 500// change to parent.height

    ListView {
        id: rootView
        anchors.fill: parent
        delegate: groupsDelegate
        model: listModel
        highlight: Rectangle {color: "lightsteelblue"; radius: 6}
//        focus: true

        Component {
            id: groupsDelegate

            Item {
                id: container
                width: 200
                height: childrenRect.height

                Column {
                    x: 14
                    id: mainColumn

                    Row {
                        id: mainRow
                        spacing: 3
                        property bool expanded: false

                        Image {
                            id: expander
                            source: "expander.png"
                            rotation: mainRow.expanded ? 90 : 0
                            opacity: elements.count === 0 ? 0 : 1
                            Behavior on rotation {
                                NumberAnimation {duration: 110}
                            }

                            MouseArea {
                                visible: expander.opacity === 1 ? true : false
                                id: expanderMouseArea
                                anchors.fill: parent
                                onClicked: {
                                    mainRow.expanded = !mainRow.expanded
                                    console.log(container.height)
                                }
                            }
                        }

                        Text {
                            id: name
                            text: group
                        }
                    }

                    Item {
                        width: 200
                        height: childView.contentHeight
                        visible: mainRow.expanded

                        ListView {
                            id: childView
                            anchors.fill: parent
//                            visible: mainRow.expanded
                            model: elements
                            delegate: groupsDelegate
                            highlight: Rectangle {color: "lightsteelblue"; radius: 5}
                            focus: true
                        }
                    }
                }

                //                                        ListView {
                //                                            anchors.right: parent.right
                //                                            visible: mainRow.expanded
                //                                            model: elements
                //                                            delegate: groupsDelegate
                //                                        }
            }
        }

        ListModel {
            id:listModel
            ListElement {group: "first"; elements: []}
            ListElement {
                group: "second"
                elements: [
                    ListElement {
                        group: "second2"
                        elements: [
                            ListElement {
                                group: "second2.2"
                                elements: []
                            }
                        ]
                    },
                    ListElement {group: "second3"; elements: []},
                    ListElement {group: "second4"; elements: []}
                ]
            }
            ListElement {group: "third"; elements: []}
        }
    }


}
