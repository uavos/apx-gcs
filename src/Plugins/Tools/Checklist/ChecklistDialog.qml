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
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Common

// checklist window: steps list on the left, selected step on the right
Window {
    id: control

    property var runner

    readonly property real fs: Style.fontSize
    readonly property real sp: Style.spacing

    title: runner && runner.title ? runner.title : qsTr("Checklist")
    transientParent: application.window
    width: fs*60
    height: fs*40
    minimumWidth: fs*28
    minimumHeight: fs*18
    color: "#101010"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    function open()
    {
        show()
        raise()
        requestActivate()
    }
    // closing the window aborts a running check
    onClosing: {
        if(runner && runner.state === "running") runner.abort()
    }

    // step shown on the right: selected by the operator, follows the running step otherwise
    property int viewed: -1
    readonly property int current: runner ? runner.current : -1
    onCurrentChanged: viewed = current
    readonly property string runState: runner ? runner.state : ""
    onRunStateChanged: if(runState === "finished") viewed = -1
    readonly property var shown: (runner && viewed >= 0 && viewed < runner.steps.length) ? runner.steps[viewed] : null
    readonly property var shownResult: (runner && viewed >= 0) ? runner.results[viewed] : undefined
    readonly property bool shownIsActive: runner && runner.state === "running" && viewed === current

    function statusText(s)
    {
        switch(s){
        case "ok": return qsTr("Passed")
        case "fail": return qsTr("Failed")
        case "skipped": return qsTr("Skipped")
        case "running": return qsTr("In progress")
        }
        return qsTr("Pending")
    }
    function statusColor(s)
    {
        switch(s){
        case "ok": return "#4cab50"
        case "fail": return "#ff5050"
        case "skipped": return "#666"
        case "running": return "#fafe86"
        }
        return "#888"
    }
    function stepStatus(index)
    {
        if(!runner) return ""
        var r = runner.results[index]
        if(r) return r.status
        return (index === current && runner.state === "running") ? "running" : ""
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: sp*2
        spacing: sp*2

        // header: procedure title from the markdown file, click shows the description page
        Text {
            Layout.fillWidth: true
            text: runner ? runner.title : ""
            color: control.viewed < 0 ? "#fff" : "#bbb"
            font: apx.font_narrow(fs*2, true)
            elide: Text.ElideRight
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: control.viewed = -1
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: sp*2

            // steps, click to read any of them
            ListView {
                id: stepsView
                Layout.preferredWidth: control.width*0.32
                Layout.fillHeight: true
                clip: true
                spacing: 1
                model: runner ? runner.steps : []
                delegate: Rectangle {
                    width: stepsView.width
                    height: fs*2.6
                    color: index === control.viewed ? "#30ffffff" : "transparent"
                    radius: sp*0.5
                    readonly property string st: stepStatus(index)
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: sp
                        spacing: sp
                        Rectangle {
                            width: fs*0.8; height: width; radius: width/2
                            color: statusColor(st)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (index + 1) + ". " + modelData.title
                            color: st === "skipped" ? "#777" : "#ddd"
                            font: apx.font_narrow(fs*1.3, index === control.current)
                            elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: control.viewed = index
                    }
                }
            }

            Rectangle { width: 1; Layout.fillHeight: true; color: "#444" }

            // selected step
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: sp*2

                Text {
                    Layout.fillWidth: true
                    visible: shown
                    text: shown ? ((viewed + 1) + ". " + shown.title) : ""
                    color: "#fff"
                    font: apx.font_narrow(fs*1.8, true)
                    wrapMode: Text.Wrap
                }
                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentHeight: bodyText.height
                    Text {
                        id: bodyText
                        width: parent.width
                        text: (shown ? shown.body : (runner ? runner.intro : "")).replace(/`([^`]+)`/g, "**$1**")
                        textFormat: Text.MarkdownText
                        color: "#ddd"
                        font: apx.font_narrow(fs*1.5)
                        wrapMode: Text.Wrap
                    }
                }
                // result of the shown step: red alert with the reason, green when passed
                Rectangle {
                    Layout.fillWidth: true
                    visible: shownResult !== undefined && shownResult.status !== "skipped"
                    readonly property bool failed: !!(shownResult && shownResult.status === "fail")
                    implicitHeight: resultColumn.implicitHeight + sp*3
                    color: failed ? "#40ff0000" : "#2040ff40"
                    border.color: failed ? "#ff5050" : "#4cab50"
                    border.width: 1
                    radius: sp
                    ColumnLayout {
                        id: resultColumn
                        anchors.fill: parent
                        anchors.margins: sp*1.5
                        spacing: sp*0.5
                        Text {
                            text: parent.parent.failed ? qsTr("FAILED") : qsTr("PASSED")
                            color: parent.parent.failed ? "#ff5050" : "#4cab50"
                            font: apx.font_narrow(fs*1.4, true)
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: text
                            text: shownResult ? shownResult.message : ""
                            color: "#eee"
                            font: apx.font_narrow(fs*1.4)
                            wrapMode: Text.Wrap
                        }
                    }
                }
                // live message of the running step
                Text {
                    Layout.fillWidth: true
                    visible: shownIsActive && runner && runner.message
                    text: runner ? runner.message : ""
                    color: "#FFBE00"
                    font: apx.font_narrow(fs*1.3)
                    wrapMode: Text.Wrap
                }

                // answers of the running confirm step
                Flow {
                    Layout.fillWidth: true
                    spacing: sp
                    visible: shownIsActive && shown && shown.type === "confirm"
                    Repeater {
                        model: (shown && shown.type === "confirm") ? (shown.buttons.length ? shown.buttons : [qsTr("Confirm")]) : []
                        TextButton {
                            text: modelData
                            size: fs*2.4
                            color: Material.color(Material.Green, Material.Shade800)
                            onTriggered: runner.answer(modelData)
                        }
                    }
                    TextButton {
                        text: qsTr("Not confirmed")
                        size: fs*2.4
                        color: Material.color(Material.Red, Material.Shade900)
                        onTriggered: runner.fail(qsTr("Not confirmed by operator"))
                    }
                }

                // one control button: start or abort
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    TextButton {
                        visible: runner && runner.state !== "running"
                        text: qsTr("Start")
                        size: fs*2.4
                        color: Material.color(Material.Blue, Material.Shade800)
                        onTriggered: runner.start()
                    }
                    TextButton {
                        visible: runner && runner.state === "running"
                        text: qsTr("Abort")
                        size: fs*2.4
                        onTriggered: runner.abort()
                    }
                }
            }
        }
    }
}
