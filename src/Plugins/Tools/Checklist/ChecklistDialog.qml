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

    readonly property real sp: ChecklistStyle.sp
    readonly property real fs: ChecklistStyle.fs

    title: runner && runner.title ? runner.title : qsTr("Checklist")
    transientParent: application.window
    width: fs*60
    height: fs*42
    minimumWidth: fs*28
    minimumHeight: fs*18
    color: ChecklistStyle.background
    Material.theme: Material.Dark

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

    // step shown on the right: a passed step selected by the operator, the running step otherwise
    property int viewed: -1
    readonly property int current: runner ? runner.current : -1
    onCurrentChanged: viewed = current
    readonly property string runState: runner ? runner.state : ""
    readonly property var shown: (runner && viewed >= 0 && viewed < runner.steps.length) ? runner.steps[viewed] : null
    readonly property var shownResult: (runner && viewed >= 0) ? runner.results[viewed] : undefined
    readonly property bool shownIsCurrent: runner && runner.state === "running" && viewed === current
    readonly property bool shownFailed: !!(shownResult && shownResult.status === "fail")
    readonly property bool shownPassed: !!(shownResult && shownResult.status === "ok")

    // only the steps already reached can be opened while running
    function selectable(index)
    {
        if(runState !== "running") return true
        return index <= current
    }
    function stepStatus(index)
    {
        if(!runner) return ""
        var r = runner.results[index]
        if(r) return r.status
        return (index === current && runner.state === "running") ? "running" : ""
    }
    function statusColor(s)
    {
        switch(s){
        case "ok": return ChecklistStyle.ok
        case "fail": return ChecklistStyle.fail
        case "running": return ChecklistStyle.running
        }
        return ChecklistStyle.pending
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: sp*2
        spacing: sp*2

        // header: procedure title, click shows the description page
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: runner ? runner.title : ""
                color: control.viewed < 0 ? ChecklistStyle.textBright : ChecklistStyle.text
                font: apx.font_narrow(ChecklistStyle.fontTitle, true)
                elide: Text.ElideRight
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: control.viewed = -1
                }
            }
            Text {
                visible: runState === "finished"
                text: runner && runner.takeoffAllowed ? qsTr("CHECK PASSED") : qsTr("CHECK FAILED")
                color: runner && runner.takeoffAllowed ? ChecklistStyle.ok : ChecklistStyle.fail
                font: apx.font_narrow(ChecklistStyle.fontStep, true)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: sp*2

            // steps: the current one is highlighted, passed ones can be opened
            ListView {
                id: stepsView
                Layout.preferredWidth: control.width*0.32
                Layout.fillHeight: true
                clip: true
                spacing: 1
                model: runner ? runner.steps : []
                delegate: Rectangle {
                    width: stepsView.width
                    height: ChecklistStyle.rowHeight
                    readonly property bool isCurrent: index === control.current && runState === "running"
                    readonly property bool isViewed: index === control.viewed
                    readonly property bool canOpen: control.selectable(index)
                    readonly property string st: stepStatus(index)
                    color: isCurrent ? ChecklistStyle.currentBg : (isViewed ? ChecklistStyle.viewedBg : "transparent")
                    radius: sp*0.5
                    Rectangle {
                        // bar marking the step being executed
                        visible: parent.isCurrent
                        width: sp*0.6
                        height: parent.height
                        radius: parent.radius
                        color: ChecklistStyle.currentMark
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: sp
                        anchors.leftMargin: sp*1.5
                        spacing: sp
                        Rectangle {
                            width: fs*0.8; height: width; radius: width/2
                            color: statusColor(st)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (index + 1) + ". " + modelData.title
                            color: canOpen ? ChecklistStyle.text : ChecklistStyle.textMuted
                            font: apx.font_narrow(ChecklistStyle.fontList, isCurrent)
                            elide: Text.ElideRight
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: parent.canOpen
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: control.viewed = index
                    }
                }
            }

            Rectangle { width: 1; Layout.fillHeight: true; color: ChecklistStyle.separator }

            // selected step
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: sp*2

                Text {
                    Layout.fillWidth: true
                    visible: shown
                    text: shown ? ((viewed + 1) + ". " + shown.title) : ""
                    color: ChecklistStyle.textBright
                    font: apx.font_narrow(ChecklistStyle.fontStep, true)
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
                        color: ChecklistStyle.text
                        font: apx.font_narrow(ChecklistStyle.fontBody)
                        wrapMode: Text.Wrap
                    }
                }
                // live message of the running step
                Text {
                    Layout.fillWidth: true
                    visible: shownIsCurrent && !shownFailed && runner && runner.message
                    text: runner ? runner.message : ""
                    color: ChecklistStyle.textInfo
                    font: apx.font_narrow(ChecklistStyle.fontSmall)
                    wrapMode: Text.Wrap
                }
                // result of the shown step: red alert with the reason, green when passed
                Rectangle {
                    Layout.fillWidth: true
                    visible: shownResult !== undefined
                    implicitHeight: resultColumn.implicitHeight + sp*3
                    color: shownFailed ? ChecklistStyle.failBg : ChecklistStyle.okBg
                    border.color: shownFailed ? ChecklistStyle.fail : ChecklistStyle.ok
                    border.width: 1
                    radius: sp
                    ColumnLayout {
                        id: resultColumn
                        anchors.fill: parent
                        anchors.margins: sp*1.5
                        spacing: sp*0.5
                        Text {
                            text: shownFailed ? qsTr("FAILED") : qsTr("PASSED")
                            color: shownFailed ? ChecklistStyle.fail : ChecklistStyle.ok
                            font: apx.font_narrow(ChecklistStyle.fontSmall, true)
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: text
                            text: shownResult ? shownResult.message : ""
                            color: ChecklistStyle.text
                            font: apx.font_narrow(ChecklistStyle.fontSmall)
                            wrapMode: Text.Wrap
                        }
                    }
                }

                // input fields requested by the step script
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: shownIsCurrent && runner && runner.form !== null
                    spacing: sp
                    Repeater {
                        id: formFields
                        model: (runner && runner.form) ? Object.keys(runner.form) : []
                        delegate: ChecklistField {
                            Layout.fillWidth: true
                            readonly property string key: modelData
                            label: modelData
                            value: String(runner.form[modelData])
                            onAccepted: control.submitForm()
                        }
                    }
                    ChecklistButton {
                        text: qsTr("Apply")
                        kind: "primary"
                        onTriggered: control.submitForm()
                    }
                }

                // buttons requested by the step script
                Flow {
                    Layout.fillWidth: true
                    spacing: sp
                    visible: shownIsCurrent && runner && runner.buttons.length > 0
                    Repeater {
                        model: runner ? runner.buttons : []
                        ChecklistButton {
                            text: modelData.text
                            kind: modelData.kind
                            onTriggered: runner.answer(modelData.text)
                        }
                    }
                }

                // control buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: sp
                    Item { Layout.fillWidth: true }
                    // restart the shown step: the current one or a passed one (then the check returns to the current step)
                    ChecklistButton {
                        visible: runState === "running" && shown && control.viewed <= control.current
                        text: shownIsCurrent ? (shownFailed ? qsTr("Retry") : qsTr("Restart step")) : qsTr("Repeat step")
                        kind: "warn"
                        onTriggered: shownIsCurrent ? runner.retry() : runner.rerun(control.viewed)
                    }
                    ChecklistButton {
                        visible: shownIsCurrent && shownFailed
                        text: qsTr("Continue")
                        kind: "neutral"
                        onTriggered: runner.proceed()
                    }
                    ChecklistButton {
                        visible: runState !== "running"
                        text: runState === "finished" ? qsTr("Restart") : qsTr("Start")
                        kind: "primary"
                        onTriggered: runner.start()
                    }
                }
            }
        }
    }

    function submitForm()
    {
        var values = {}
        for (var i = 0; i < formFields.count; ++i) {
            var item = formFields.itemAt(i)
            if(item) values[item.key] = item.value
        }
        runner.submit(values)
    }
}
