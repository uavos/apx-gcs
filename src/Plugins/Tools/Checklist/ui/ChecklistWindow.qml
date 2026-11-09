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

// Checklist window: steps list on the left, selected step on the right.
// Displays `checklist` (ChecklistPlugin: doc, results, buttons, form, message) and calls its
// start/abort/answer/submit/retry/rerun/proceed; knows nothing about the procedure format.
Window {
    id: control

    property var checklist

    ChecklistTheme { id: theme }
    readonly property real sp: theme.sp
    readonly property real fs: theme.fs

    title: checklist.doc.title ? checklist.doc.title : qsTr("Checklist")
    transientParent: application.window
    width: fs*60
    height: fs*42
    minimumWidth: fs*28
    minimumHeight: fs*18
    color: theme.background
    Material.theme: Material.Dark

    visible: true
    Component.onCompleted: {
        raise()
        requestActivate()
    }
    // closing the window aborts a running check
    onClosing: checklist.abort()

    // step shown on the right: a passed step selected by the operator, the running step otherwise
    readonly property var steps: checklist.steps
    readonly property int current: checklist.current
    readonly property string phase: checklist.phase
    property int viewed: current
    onCurrentChanged: viewed = current
    onStepsChanged: viewed = -1

    readonly property var shown: (viewed >= 0 && viewed < steps.length) ? steps[viewed] : null
    readonly property var shownResult: viewed >= 0 ? checklist.results[viewed] : undefined
    readonly property bool shownIsCurrent: phase === "running" && viewed === current
    readonly property bool shownFailed: !!(shownResult && shownResult.status === "fail")

    // only the steps already reached can be opened while running
    function selectable(index)
    {
        return phase !== "running" || index <= current
    }
    function stepStatus(index)
    {
        var r = checklist.results[index]
        if(r) return r.status
        return (index === current && phase === "running") ? "running" : ""
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
                text: checklist.doc.title
                color: control.viewed < 0 ? theme.textBright : theme.text
                font: apx.font_narrow(theme.fontTitle, true)
                elide: Text.ElideRight
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: control.viewed = -1
                }
            }
            Text {
                visible: phase === "finished"
                text: checklist.takeoffAllowed ? qsTr("CHECK PASSED") : qsTr("CHECK FAILED")
                color: checklist.takeoffAllowed ? theme.ok : theme.fail
                font: apx.font_narrow(theme.fontStep, true)
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
                model: control.steps
                delegate: Rectangle {
                    width: stepsView.width
                    height: theme.rowHeight
                    readonly property bool isCurrent: index === control.current && phase === "running"
                    readonly property bool canOpen: control.selectable(index)
                    color: isCurrent ? theme.currentBg : (index === control.viewed ? theme.viewedBg : "transparent")
                    radius: sp*0.5
                    Rectangle {
                        // bar marking the step being executed
                        visible: parent.isCurrent
                        width: sp*0.6
                        height: parent.height
                        radius: parent.radius
                        color: theme.currentMark
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: sp
                        anchors.leftMargin: sp*1.5
                        spacing: sp
                        Rectangle {
                            width: fs*0.8; height: width; radius: width/2
                            color: theme.statusColor(stepStatus(index))
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (index + 1) + ". " + modelData.title
                            color: canOpen ? theme.text : theme.textMuted
                            font: apx.font_narrow(theme.fontList, isCurrent)
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

            Rectangle { width: 1; Layout.fillHeight: true; color: theme.separator }

            // selected step: everything flows top-down right under the description
            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentHeight: stepColumn.implicitHeight
                ColumnLayout {
                    id: stepColumn
                    width: parent.width
                    spacing: sp*2

                    Text {
                        Layout.fillWidth: true
                        visible: shown
                        text: shown ? ((viewed + 1) + ". " + shown.title) : ""
                        color: theme.textBright
                        font: apx.font_narrow(theme.fontStep, true)
                        wrapMode: Text.Wrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (shown ? shown.body : checklist.doc.intro).replace(/`([^`]+)`/g, "**$1**")
                        textFormat: Text.MarkdownText
                        color: theme.text
                        font: apx.font_narrow(theme.fontBody)
                        wrapMode: Text.Wrap
                    }
                    // live message of the running step
                    Text {
                        Layout.fillWidth: true
                        visible: shownIsCurrent && !shownFailed && checklist.message
                        text: checklist.message
                        color: theme.textInfo
                        font: apx.font_narrow(theme.fontSmall)
                        wrapMode: Text.Wrap
                    }
                    // result of the shown step: red alert with the reason, green when passed
                    Rectangle {
                        Layout.fillWidth: true
                        visible: shownResult !== undefined
                        implicitHeight: resultColumn.implicitHeight + sp*3
                        color: shownFailed ? theme.failBg : theme.okBg
                        border.color: shownFailed ? theme.fail : theme.ok
                        border.width: 1
                        radius: sp
                        ColumnLayout {
                            id: resultColumn
                            anchors.fill: parent
                            anchors.margins: sp*1.5
                            spacing: sp*0.5
                            Text {
                                text: shownFailed ? qsTr("FAILED") : qsTr("PASSED")
                                color: shownFailed ? theme.fail : theme.ok
                                font: apx.font_narrow(theme.fontSmall, true)
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: text
                                text: shownResult ? shownResult.message : ""
                                color: theme.text
                                font: apx.font_narrow(theme.fontSmall)
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    // input fields requested by the step script
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: shownIsCurrent && checklist.form !== null
                        spacing: sp
                        Repeater {
                            id: formFields
                            model: checklist.form ? Object.keys(checklist.form) : []
                            delegate: ChecklistField {
                                Layout.fillWidth: true
                                readonly property string key: modelData
                                label: modelData
                                value: String(checklist.form[modelData])
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
                        visible: shownIsCurrent && checklist.buttons.length > 0
                        Repeater {
                            model: checklist.buttons
                            ChecklistButton {
                                text: modelData.text
                                kind: modelData.kind
                                onTriggered: checklist.answer(modelData.text)
                            }
                        }
                    }

                    // control buttons
                    Flow {
                        Layout.fillWidth: true
                        spacing: sp
                        // retry the failed step, or repeat a passed one (then the check returns to the current step)
                        ChecklistButton {
                            visible: phase === "running" && shown && (shownFailed || control.viewed < control.current)
                            text: shownIsCurrent ? qsTr("Retry") : qsTr("Repeat step")
                            kind: "warn"
                            onTriggered: shownIsCurrent ? checklist.retry() : checklist.rerun(control.viewed)
                        }
                        ChecklistButton {
                            visible: shownIsCurrent && shownFailed
                            text: qsTr("Continue")
                            kind: "neutral"
                            onTriggered: checklist.proceed()
                        }
                        ChecklistButton {
                            visible: phase !== "running"
                            text: phase === "finished" ? qsTr("Restart") : qsTr("Start")
                            kind: "primary"
                            onTriggered: checklist.start()
                        }
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
        checklist.submit(values)
    }
}
