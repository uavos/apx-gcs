pragma ComponentBehavior: Bound

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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtCore

import Apx.Common
import Apx.Menu

Rectangle {
    id: control

    implicitHeight: layout.implicitHeight
    implicitWidth: layout.implicitWidth

    border.width: 0
    color: "#000000"

    property string selectedPage: ""
    property bool pageStateRefreshPending: false
    property var pageStates: []
    property var globalUi: ui
    property var globalApx: apx
    property var prefsStore: application.prefs
    property real uiScale: control.globalUi ? control.globalUi.scale : 1

    readonly property var pages: signalsModel.pages
    readonly property var pinnedPages: signalsModel.pinnedPages
    readonly property int activePageIndex: checkedPageIndex()
    readonly property var activePage: pageAt(activePageIndex)
    readonly property real pinnedChartHeight: Math.max(44 * uiScale,
                                                       Math.min(96 * uiScale,
                                                                (control.height - bottomArea.implicitHeight)
                                                                / Math.max(pinnedPages.length + 1, 2)))

    SignalsModel {
        id: signalsModel
        prefsAdapter: control.prefsStore

        onSettingsLoaded: control.handleModelChanged()
        onSettingsSaved: control.handleModelChanged()
    }

    Component {
        id: factPopupComponent

        FactMenuPopup {
            pinned: true
        }
    }

    Settings {
        category: "signals"
        property alias page: control.selectedPage
    }

    Connections {
        target: control.globalApx ? control.globalApx.fleet.current.mandala : null

        function onTelemetryDecoded()
        {
            control.schedulePageStateRefresh()
        }
    }

    Component.onCompleted: handleModelChanged()

    function createEditorPopup(pos)
    {
        var popupParent = control.globalUi && control.globalUi.window ? control.globalUi.window
                                                                      : control

        return factPopupComponent.createObject(popupParent, {
                                                   "pos": pos
                                               })
    }

    function createPopupFact(popup, url, properties)
    {
        var component = Qt.createComponent(Qt.resolvedUrl(url))
        if (component.status !== Component.Ready) {
            console.log(component.errorString())
            popup.destroy()
            return null
        }

        var fact = component.createObject(popup, properties)
        component.destroy()

        if (!fact) {
            popup.destroy()
            return null
        }

        popup.showFact(fact)
        popup.open()
        return fact
    }

    function openSetsEditor()
    {
        var popup = createEditorPopup(Qt.point(0.76, 0.08))
        if (!popup)
            return

        createPopupFact(popup, "MenuSets.qml", {
                            "signalsModel": signalsModel
        })
    }

    function pageAt(index)
    {
        return index >= 0 && index < pages.length ? pages[index] : null
    }

    function checkedPageIndex()
    {
        var button = buttonGroup.checkedButton
        if (!button)
            return -1

        var index = button["pageIndex"]
        return index === undefined ? -1 : Number(index)
    }

    function pageName(index)
    {
        var page = pageAt(index)
        if (page && page.name)
            return String(page.name)
        return signalsModel.defaultPageTitle(Math.max(index, 0))
    }

    function pageFacts(index)
    {
        var page = pageAt(index)
        return page && page.items instanceof Array ? page.items : []
    }

    function pageTitle(page, fallbackIndex)
    {
        if (page && page.name)
            return String(page.name)
        return signalsModel.defaultPageTitle(Math.max(fallbackIndex, 0))
    }

    function activeSetTitle()
    {
        if (signalsModel.activeSet && signalsModel.activeSet.title)
            return String(signalsModel.activeSet.title)
        return ""
    }

    function pageIndexFor(page)
    {
        for (var i = 0; i < pages.length; ++i) {
            if (pages[i] === page)
                return i
        }

        return -1
    }

    function pinnedPageIndex(pinnedIndex)
    {
        var match = 0

        for (var i = 0; i < pages.length; ++i) {
            if (!pages[i] || !pages[i].pin)
                continue

            if (match === pinnedIndex)
                return i

            match++
        }

        return -1
    }

    function overlayFont(pixelSize)
    {
        var size = Math.max(9, pixelSize)
        if (control.globalApx && typeof control.globalApx.font_narrow === "function")
            return control.globalApx.font_narrow(size)
        return Qt.font({"pixelSize": size})
    }

    function pageSpeedValue(page)
    {
        if (!page)
            return 1.0

        if (signalsModel && typeof signalsModel.normalizeSpeed === "function")
            return signalsModel.normalizeSpeed(page.speed)

        return Number(page.speed)
    }

    function pageSpeedText(page)
    {
        var speedValue = pageSpeedValue(page)
        if (speedValue === 1.0)
            return ""

        if (signalsModel && typeof signalsModel.formatSpeed === "function")
            return signalsModel.formatSpeed(speedValue)

        var text = String(speedValue)
        if (text.endsWith(".0"))
            text = text.slice(0, -2)
        return text + "x"
    }

    function normalizeBindText(value)
    {
        var text = String(value === undefined || value === null ? "" : value).trim()
        var simplePath = /^(?:mandala\.)?[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+(?:\.value)?$/

        if (text === "" || !simplePath.test(text))
            return text

        if (text.startsWith("mandala."))
            text = text.slice(8)
        if (text.endsWith(".value"))
            text = text.slice(0, -6)

        return text
    }

    function itemBind(item)
    {
        if (!item || item.bind === undefined)
            return ""

        return normalizeBindText(item.bind)
    }

    function defaultItemColor(index)
    {
        return Material.color(Material.Blue + index * 2)
    }

    function itemColor(item, index)
    {
        if (item && item.color !== undefined && String(item.color).trim() !== "")
            return item.color
        return defaultItemColor(index)
    }

    function itemTitle(item, index)
    {
        if (item)
            return itemBind(item)
        return qsTr("Item") + " " + (index + 1)
    }

    function escapeHtml(text)
    {
        return String(text)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
    }

    function evaluateBinding(bind)
    {
        try {
            return eval(normalizeBindText(bind))
        } catch (error) {
            return NaN
        }
    }

    function evaluateCondition(expression, value, item, page)
    {
        if (!expression || String(expression).trim() === "")
            return false

        var rawValue = value
        var bind = itemBind(item)
        var title = itemTitle(item, 0)
        var pageNameValue = page && page.name ? page.name : ""

        try {
            return !!eval(expression)
        } catch (error) {
            return false
        }
    }

    function evaluatePageState(index)
    {
        var page = pageAt(index)
        var state = {
            "warning": false,
            "messages": [],
            "toolTip": ""
        }

        if (!page)
            return state

        var lines = ["<b>" + escapeHtml(pageName(index)) + "</b>"]
        var items = page.items instanceof Array ? page.items : []

        for (var i = 0; i < items.length; ++i) {
            var item = items[i]
            var title = itemTitle(item, i)
            lines.push("<font color='" + String(itemColor(item, i)) + "'>\u25A0</font> "
                       + escapeHtml(title))

            var value = evaluateBinding(itemBind(item))
            if (item.warning && evaluateCondition(item.warning, value, item, page)) {
                state.warning = true
                state.messages.push(qsTr("Warning") + ": " + title + " (" + item.warning + ")")
            }
        }

        if (state.messages.length > 0) {
            lines.push("<br><b>" + escapeHtml(qsTr("Alerts")) + "</b>")
            for (var j = 0; j < state.messages.length; ++j)
                lines.push(escapeHtml(state.messages[j]))
        }

        state.toolTip = lines.join("<br>")
        return state
    }

    function refreshPageStates()
    {
        var states = []
        for (var i = 0; i < pages.length; ++i)
            states.push(evaluatePageState(i))
        pageStates = states
    }

    function schedulePageStateRefresh()
    {
        if (pageStateRefreshPending)
            return

        pageStateRefreshPending = true
        Qt.callLater(function() {
            pageStateRefreshPending = false
            refreshPageStates()
        })
    }

    function pageState(index)
    {
        return index >= 0 && index < pageStates.length
               ? pageStates[index]
               : {
                     "warning": false,
                     "messages": [],
                     "toolTip": ""
                 }
    }

    function selectSavedPage()
    {
        if (pages.length <= 0) {
            selectedPage = ""
            signals.facts = []
            return
        }

        if (selectedPage !== "") {
            for (var i = 0; i < pages.length; ++i) {
                if (pageName(i) === selectedPage)
                    return
            }
        }

        selectedPage = pageName(0)
    }

    function handleModelChanged()
    {
        Qt.callLater(function() {
            selectSavedPage()
            Qt.callLater(function() {
                selectSavedPage()
                schedulePageStateRefresh()
            })
        })
    }

    function cyclePageSpeed()
    {
        if (!activePage)
            return

        signalsModel.setPageSpeed(activePageIndex,
                                  signalsModel.nextSpeedValue(activePage.speed))
    }

    function cycleSpeedForPage(page)
    {
        var index = pageIndexFor(page)
        if (index < 0)
            return

        signalsModel.setPageSpeed(index,
                                  signalsModel.nextSpeedValue(page.speed))
    }

    function cycleSpeedForPinnedPage(pinnedIndex, page)
    {
        var index = pinnedPageIndex(pinnedIndex)
        if (index < 0 && page)
            index = pageIndexFor(page)
        if (index < 0)
            return

        var targetPage = pageAt(index)
        if (!targetPage)
            return

        signalsModel.setPageSpeed(index,
                                  signalsModel.nextSpeedValue(targetPage.speed))
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 2 * control.uiScale

        Repeater {
            model: control.pinnedPages

            delegate: Item {
                id: pinnedChartPane

                required property var modelData
                required property int index

                Layout.fillWidth: true
                Layout.preferredHeight: control.pinnedChartHeight
                Layout.minimumHeight: 36 * control.uiScale
                clip: true

                SignalsView {
                    anchors.fill: parent
                    uiContext: control.globalUi
                    apxContext: control.globalApx
                    facts: pinnedChartPane.modelData && pinnedChartPane.modelData.items instanceof Array
                           ? pinnedChartPane.modelData.items
                           : []
                    speed: signalsModel.speedIndex(pinnedChartPane.modelData
                                                   ? pinnedChartPane.modelData.speed
                                                   : 1.0)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: control.cycleSpeedForPinnedPage(pinnedChartPane.index,
                                                               pinnedChartPane.modelData)
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: 4 * control.uiScale
                    anchors.rightMargin: 4 * control.uiScale
                    radius: 2 * control.uiScale
                    color: "#A0000000"
                    visible: titleLabel.text !== ""

                    implicitWidth: titleColumn.implicitWidth + 10 * control.uiScale
                    implicitHeight: titleColumn.implicitHeight + 4 * control.uiScale

                    Column {
                        id: titleColumn
                        anchors.centerIn: parent
                        spacing: 1 * control.uiScale

                        Label {
                            id: titleLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: control.pageTitle(pinnedChartPane.modelData, pinnedChartPane.index)
                            color: "#FFFFFF"
                            font: control.overlayFont(10 * control.uiScale)
                        }

                        Label {
                            id: titleSpeedLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: text !== ""
                            text: control.pageSpeedText(pinnedChartPane.modelData)
                            color: "#CCCCCC"
                            font: control.overlayFont(8 * control.uiScale)
                        }
                    }
                }
            }
        }

        Item {
            id: mainChartArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130 * control.uiScale
            clip: true

            SignalsView {
                id: signals
                anchors.fill: parent
                uiContext: control.globalUi
                apxContext: control.globalApx
                facts: []
                speed: control.activePage
                       ? signalsModel.speedIndex(control.activePage.speed)
                       : signalsModel.speedIndex(1.0)
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: control.cyclePageSpeed()
            }

            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 4 * control.uiScale
                anchors.rightMargin: 4 * control.uiScale
                radius: 2 * control.uiScale
                color: "#A0000000"
                visible: mainTitleLabel.text !== ""

                implicitWidth: mainTitleColumn.implicitWidth + 10 * control.uiScale
                implicitHeight: mainTitleColumn.implicitHeight + 4 * control.uiScale

                Column {
                    id: mainTitleColumn
                    anchors.centerIn: parent
                    spacing: 1 * control.uiScale

                    Label {
                        id: mainTitleLabel
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: control.activePage ? control.pageTitle(control.activePage, control.activePageIndex) : ""
                        color: "#FFFFFF"
                        font: control.overlayFont(10 * control.uiScale)
                    }

                    Label {
                        id: mainSpeedLabel
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: text !== ""
                        text: control.pageSpeedText(control.activePage)
                        color: "#CCCCCC"
                        font: control.overlayFont(8 * control.uiScale)
                    }
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 4 * control.uiScale
                anchors.bottomMargin: 4 * control.uiScale
                radius: 2 * control.uiScale
                color: "#A0000000"
                visible: mainSetLabel.text !== ""

                implicitWidth: mainSetLabel.implicitWidth + 10 * control.uiScale
                implicitHeight: mainSetLabel.implicitHeight + 4 * control.uiScale

                Label {
                    id: mainSetLabel
                    anchors.centerIn: parent
                    text: control.activeSetTitle()
                    color: "#FFFFFF"
                    font: control.overlayFont(10 * control.uiScale)
                }
            }

            Label {
                anchors.centerIn: parent
                visible: control.pages.length <= 0
                text: qsTr("No pages in the active set.")
                color: Material.secondaryTextColor
            }
        }

        ButtonGroup {
            id: buttonGroup

            onCheckedButtonChanged: {
                if (checkedButton) {
                    control.selectedPage = checkedButton.text
                    signals.facts = Qt.binding(function() {
                        return checkedButton ? checkedButton.values : []
                    })
                } else {
                    signals.facts = []
                }
            }
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24 * control.uiScale

            Repeater {
                id: tabsRepeater
                model: control.pages

                onItemAdded: function(index, item) {
                    if (index === tabsRepeater.count - 1)
                        Qt.callLater(control.selectSavedPage)
                }

                delegate: SignalButton {
                    required property int index

                    property int pageIndex: index

                    text: control.pageName(index)
                    checked: control.selectedPage !== ""
                             ? text === control.selectedPage
                             : index === 0
                    values: control.pageFacts(index)
                    pageToolTip: control.pageState(index).toolTip
                    pageWarning: control.pageState(index).warning
                }
            }

            IconButton {
                iconName: "plus"
                toolTip: qsTr("Edit chart configuration")
                Layout.fillHeight: true
                Layout.minimumWidth: height
                onTriggered: control.openSetsEditor()
            }
        }
    }
}
