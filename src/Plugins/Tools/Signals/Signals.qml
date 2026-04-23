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

import Apx.Common

// Root widget for the Signals plugin.
// Accessible from child QML files via id: signalsWidget (context property set below).
Rectangle {
    id: signalsWidget

    SignalsModel {
        id: signalsModel
    }

    implicitHeight: mainLayout.implicitHeight
    implicitWidth: mainLayout.implicitWidth
    border.width: 0
    color: "#000"

    // Active pages — array of MenuPage Fact objects rebuilt from the active set
    property var activePages: []
    // Currently selected (non-pinned) page
    property var currentPage: null
    // Pinned pages shown stacked
    property var pinnedPages: []
    // Title of the active set
    property string activeSetTitle: ""
    // Index of the active set in the JSON
    property int activeSetIndex: 0

    // -----------------------------------------------------------------
    // Public API — called from child components (MenuItem, MenuPage, etc.)
    // -----------------------------------------------------------------

    function saveSettings() {
        var json = signalsModel.loadJson();
        json.sets = [];
        var activeSets = _buildSetsFromPages();
        json.sets = activeSets.sets;
        if (!json.active) json.active = {};
        json.active["signals"] = activeSetIndex;
        signalsModel.saveJson(json);
    }

    function checkScrMatches(val) {
        if (!val || val === "") return false;
        var matches = false;
        for (var i = 0; i < activePages.length; ++i) {
            if (activePages[i].checkScrs(val)) matches = true;
        }
        return matches;
    }

    function updateLayout() {
        pinnedPages = activePages.filter(function(p) { return p.pinned; });
        var nonPinnedPages = activePages.filter(function(p) { return !p.pinned; });
        if (currentPage && currentPage.pinned)
            currentPage = nonPinnedPages.length > 0 ? nonPinnedPages[0] : null;
        if (!currentPage && nonPinnedPages.length > 0)
            currentPage = nonPinnedPages[0];
    }

    function updateSeriesColors() {
        singleChart.updateSeriesColor();
        for (var i = 0; i < pinnedChartsRepeater.count; ++i) {
            var item = pinnedChartsRepeater.itemAt(i);
            if (item)
                item.updateSeriesColor();
        }
    }

    function activatePage(page) {
        if (!page) return;
        if (page.pinned)
            return;
        currentPage = page;
        singleChart.resetEnable = true;
        singleChart.facts = Qt.binding(function() {
            return signalsWidget.currentPage ? signalsWidget.currentPage.values : [];
        });
        singleChart.speedFactorValue = Qt.binding(function() {
            return signalsWidget.currentPage ? signalsWidget.currentPage.speed : 1.0;
        });
    }

    // -----------------------------------------------------------------
    // Initialization
    // -----------------------------------------------------------------

    Component.onCompleted: {
        loadSettings();
    }

    function loadSettings() {
        // Destroy old page facts
        _destroyActivePages();

        var json = signalsModel.loadJson();
        var sets = json.sets;
        var idx = signalsModel.activeIndex(json);

        activeSetIndex = idx;
        var activeSet = sets[idx];
        activeSetTitle = activeSet.title || qsTr("default");

        var pages = activeSet.pages || [];
        var newPages = [];
        for (var i = 0; i < pages.length && i < 10; ++i) {
            var pg = _createMenuPage(pages[i]);
            if (pg) newPages.push(pg);
        }
        activePages = newPages;
        pinnedPages = newPages.filter(function(p) { return p.pinned; });
        var nonPinnedPages = newPages.filter(function(p) { return !p.pinned; });
        currentPage = nonPinnedPages.length > 0 ? nonPinnedPages[0] : null;

        if (currentPage) {
            singleChart.resetEnable = true;
            singleChart.facts = Qt.binding(function() {
                return signalsWidget.currentPage ? signalsWidget.currentPage.values : [];
            });
            singleChart.speedFactorValue = Qt.binding(function() {
                return signalsWidget.currentPage ? signalsWidget.currentPage.speed : 1.0;
            });
        } else {
            singleChart.facts = [];
            singleChart.speedFactorValue = 1.0;
        }
    }

    function _loadJson() {
        return signalsModel.loadJson();
    }

    function _destroyActivePages() {
        for (var i = 0; i < activePages.length; ++i) {
            if (activePages[i] && typeof activePages[i].deleteFact === 'function')
                activePages[i].deleteFact();
        }
        activePages = [];
    }

    function _createMenuPage(pageData) {
        var component = Qt.createComponent("MenuPage.qml");
        if (component.status !== Component.Ready) {
            console.warn("Signals: cannot load MenuPage.qml: " + component.errorString());
            return null;
        }
        var pg = component.createObject(signalsWidget, {
            "title": pageData.name ? pageData.name : qsTr("P"),
            "isDirectEdit": true
        });
        pg.parentFact = apx.fleet.local;
        pg.load(pageData);
        return pg;
    }

    // Collect current page data back into sets JSON structure
    function _buildSetsFromPages() {
        var savedPages = [];
        for (var i = 0; i < activePages.length; ++i)
            savedPages.push(activePages[i].save());

        // Load existing sets, replace active one
        var json = signalsModel.loadJson();
        var sets = (json && json.sets) ? JSON.parse(JSON.stringify(json.sets)) : [];
        if (sets.length === 0) sets.push({ title: activeSetTitle, pages: [] });
        if (activeSetIndex >= sets.length) activeSetIndex = 0;
        sets[activeSetIndex].pages = savedPages;
        sets[activeSetIndex].title = activeSetTitle;
        return { sets: sets };
    }

    // -----------------------------------------------------------------
    // Telemetry update
    // -----------------------------------------------------------------

    Connections {
        target: apx.fleet.current.mandala
        function onTelemetryDecoded() {
            for (var i = 0; i < activePages.length; ++i)
                activePages[i].updateChartsValues();
        }
    }

    // -----------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0

        // Pinned pages stacked above the main view
        Repeater {
            id: pinnedChartsRepeater
            model: pinnedPages

            delegate: ChartsView {
                Layout.fillWidth: true
                Layout.preferredHeight: 80 * ui.scale
                Layout.minimumHeight: 20
                facts: modelData.values
                speedFactorValue: modelData.speed
            }
        }

        // Main (non-pinned) single chart
        ChartsView {
            id: singleChart
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130 * ui.scale
        }

        ButtonGroup {
            id: pageButtonGroup
        }

        // Bottom bar: page tabs, set label, speed button
        RowLayout {
            id: bottomBar
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24 * ui.scale

            Repeater {
                id: pageTabsRepeater
                model: activePages

                delegate: PageButton {
                    page: modelData
                    text: modelData.title
                    Layout.fillHeight: true
                    ButtonGroup.group: pageButtonGroup
                    checked: !modelData.pinned && modelData === signalsWidget.currentPage
                    onClicked: {
                        var wasCurrent = modelData === signalsWidget.currentPage;
                        if (modelData.pinned) {
                            if (modelData)
                                modelData.trigger();
                        } else if (wasCurrent) {
                            // already active — open page editor
                            if (modelData) modelData.trigger();
                        } else {
                            signalsWidget.activatePage(modelData);
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Set name label — click opens sets editor
            TextButton {
                text: activeSetTitle || qsTr("default")
                Layout.fillHeight: true
                Layout.minimumWidth: height * 3
                toolTip: qsTr("Click to edit chart sets")
                onClicked: openSetsEditor()
            }

            // Speed button — cycles per-page speed factor
            TextButton {
                text: currentPage ? (currentPage.speed + "x") : "1x"
                Layout.fillHeight: true
                Layout.minimumWidth: height * 3
                toolTip: qsTr("Chart scroll speed")
                onClicked: {
                    if (!currentPage) return;
                    var factors = [0.2, 0.5, 1, 2, 4];
                    var idx = factors.indexOf(currentPage.speed);
                    var next = (idx >= 0 && idx < factors.length - 1)
                               ? factors[idx + 1] : factors[0];
                    currentPage.setSpeed(next);
                    saveSettings();
                }
            }
        }
    }

    // + button (top-right) — opens current page editor
    IconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Style.spacing
        size: Style.buttonSize * 0.7
        iconName: "plus"
        toolTip: qsTr("Edit current page")
        opacity: ui.effects ? (hovered ? 1 : 0.5) : 1
        onTriggered: {
            if (currentPage)
                currentPage.trigger();
        }
    }

    // -----------------------------------------------------------------
    // Sets editor popup
    // -----------------------------------------------------------------

    property var setsEditorPopup: null

    function openSetsEditor() {
        if (setsEditorPopup) return;
        var c = Qt.createComponent("SignalsMenuPopup.qml", Component.PreferSynchronous, ui.window);
        if (c.status === Component.Ready) {
            var obj = c.createObject(ui.window);
            setsEditorPopup = obj;
            obj.accepted.connect(function() { loadSettings(); });
            obj.closed.connect(function() { setsEditorPopup = null; });
            obj.open();
        } else {
            console.warn("Signals: cannot open sets editor: " + c.errorString());
        }
    }

    // -----------------------------------------------------------------
    // Legacy migration
    // -----------------------------------------------------------------

    function _migrateLegacy(oldJson) {
        return signalsModel.migrateLegacy(oldJson);
    }

    // -----------------------------------------------------------------
    // Default set (mirrors the hardcoded buttons from the old Signals.qml)
    // -----------------------------------------------------------------

    function _buildDefaultSet() {
        return signalsModel.buildDefaultSet();
    }
}
