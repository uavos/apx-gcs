/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

import APX.Facts 1.0

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: 600
    implicitHeight: 300

    property real lineSize: Style.buttonSize*0.4
    property real lineSpace: 0

    ColumnLayout {
        spacing: 0
        anchors.margins: Style.spacing
        anchors.fill: parent


        LogListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredWidth: 300
            Layout.preferredHeight: 400
            currentIndex: -1

            property list<Item> selectedItems

            spacing: lineSpace

            Menu {
                id: contextMenu
                MenuItem {
                    text: "Cut"
                    enabled: listView.footerItem.isSomeSelection()
                    onTriggered: {
                        listView.footerItem.cut()
                    }
                }
                MenuItem {
                    text: "Copy"
                    enabled: listView.footerItem.isSomeSelection() || (listView.currentIndex !== -1)
                    onTriggered: {
                        if (listView.currentIndex !== -1)
                            listView.copySelectedLines()
                        else
                            listView.footerItem.copy()
                    }
                }
                MenuItem {
                    text: "Paste"
                    enabled: !listView.footerItem.isClipboardEmpty
                    onTriggered: {
                        listView.footerItem.paste()
                        listView.footerItem.setFocus()
                    }
                }
                MenuItem {
                    text: "Copy all"
                    onTriggered: {
                        listView.footerItem.copyAll()
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: contextMenu.popup()
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.MiddleButton
                onWheel: {
                    if (wheel.angleDelta.y > 0)
                        listView.focus=false
                    wheel.accepted=false
                }
            }

            model: application.notifyModel
            delegate: TerminalLine {
                width: listView.width
                text: model.text
                subsystem: model.subsystem
                source: model.source
                type: model.type
                options: model.options
                fact: model.fact
                timestamp: model.timestamp
            }

            add: Transition {
                id: transAdd
                enabled: ui.smooth
                NumberAnimation {
                    properties: "x";
                    from: -listView.width;
                    duration: transAdd.ViewTransition.item.source==AppNotify.FromInput?0:150
                    easing.type: Easing.OutCubic
                }
            }

            onCountChanged: scrollTimer.start()
            Timer {
                id: scrollTimer
                interval: 1
                onTriggered: listView.scrollToEnd()
            }

            Keys.onPressed: {
                if (event.key === Qt.Key_C && (event.modifiers & (Qt.ControlModifier | Qt.MetaModifier))) {
                    copySelectedLines()
                }
                if ((event.text.length == 1 && event.modifiers == Qt.NoModifier) || event.key == Qt.Key_Backspace) {
                    event.accepted=true
                    footerItem.setFocus();
                    if (event.key != Qt.Key_Backspace)
                        footerItem.appendCmd(event.text)
                    else
                        footerItem.doBackSpace()
                }
            }
            Keys.onTabPressed: {
                event.accepted=true
                footerItem.hints()
                footerItem.setFocus()
            }
            Keys.onEnterPressed: {
                event.accepted = true
                footerItem.exec()
            }
            Keys.onReturnPressed: {
                event.accepted = true
                footerItem.exec()
            }
            Keys.onUpPressed: {
                footerItem.setFocus()
                footerItem.upPressed(event)
            }
            Keys.onDownPressed: {
                footerItem.setFocus()
                footerItem.downPressed(event)
            }

            footer: TerminalExec {
                width: parent.width
                onFocused: {
                    listView.scrollToEnd()
                    listView.currentIndex = -1

                    for (var i = 0; i < listView.selectedItems.length; i++) {
                        listView.selectedItems[i].selected=false
                    }
                    listView.selectedItems.length=0
                }
            }

            function copySelectedLines()
            {
                var selectedText = ""
                for (var i = 0; i < selectedItems.length; i++) {
                    selectedText += selectedItems[i].text + '\n'
                }
                footerItem.copyText(selectedText)
            }
        }
    }

    Item {
        anchors {
            top: parent.top
            right: parent.right
            rightMargin: 35
            topMargin: 15
        }

        Rectangle {
            width: 25
            height: 20
            color: "#80808080"

            Text {
                anchors.centerIn: parent
                text: "☰"
                color: "white"
                font.pixelSize: 12
            }

            MouseArea {
                anchors.fill: parent
                onClicked: contextMenu.popup()
            }
        }
    }
}
