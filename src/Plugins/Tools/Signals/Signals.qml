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
        var json = _loadJson();
        json.sets = [];
        var activeSets = _buildSetsFromPages();
        json.sets = activeSets.sets;
        if (!json.active) json.active = {};
        json.active["signals"] = activeSetIndex;
        application.prefs.saveFile("signals.json", JSON.stringify(json, ' ', 2));
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
    }

    function updateSeriesColors() {
        singleChart.updateSeriesColor();
    }

    function activatePage(page) {
        if (!page) return;
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

        var json = _loadJson();

        // Legacy migration: {page, signalas} -> {active, sets}
        if (json && json.signalas && !json.sets)
            json = _migrateLegacy(json);

        var sets = (json && json.sets) ? json.sets : [];
        if (sets.length === 0) sets = [_buildDefaultSet()];

        var idx = 0;
        if (json && json.active)
            idx = json.active["signals"] || 0;
        if (idx < 0 || idx >= sets.length) idx = 0;

        activeSetIndex = idx;
        var activeSet = sets[idx];
        activeSetTitle = activeSet.title || "default";

        var pages = activeSet.pages || [];
        var newPages = [];
        for (var i = 0; i < pages.length && i < 10; ++i) {
            var pg = _createMenuPage(pages[i]);
            if (pg) newPages.push(pg);
        }
        activePages = newPages;
        pinnedPages = newPages.filter(function(p) { return p.pinned; });
        currentPage = newPages.length > 0 ? newPages[0] : null;

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
        }
    }

    function _loadJson() {
        var f = application.prefs.loadFile("signals.json");
        return f ? JSON.parse(f) : {};
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
            "title": pageData.name ? pageData.name : "P",
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
        var json = _loadJson();
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
                    checked: modelData === signalsWidget.currentPage
                    onClicked: {
                        if (checked) {
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
                    currentPage.speed = next;
                    singleChart.speedFactorValue = next;
                    saveSettings();
                }
            }
        }
    }

    // + button (top-right) — opens current page item editor
    IconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Style.spacing
        size: Style.buttonSize * 0.7
        iconName: "plus"
        toolTip: qsTr("Edit current page items")
        opacity: ui.effects ? (hovered ? 1 : 0.5) : 1
        onTriggered: {
            if (currentPage)
                currentPage.addNewItem();
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
        var items = (oldJson.signalas || [])
            .map(function(it) {
                return {
                    bind: it.bind || it.name || "",
                    title: it.title || "",
                    color: it.color || "",
                    filters: [],
                    warn: it.warn || "",
                    alarm: it.alarm || "",
                    act: it.act || "",
                    save: it.save || ""
                };
            })
            .filter(function(it) { return it.bind !== ""; });

        return {
            active: { signals: 0 },
            sets: [{
                title: "default",
                pages: [{
                    name: oldJson.page || "page 1",
                    pin: false,
                    speed: 1.0,
                    items: items
                }]
            }]
        };
    }

    // -----------------------------------------------------------------
    // Default set (mirrors the hardcoded buttons from the old Signals.qml)
    // -----------------------------------------------------------------

    function _buildDefaultSet() {
        return {
            title: "default",
            pages: [
                { name: "R",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.att.roll.value",  title: "roll cmd" },
                    { bind: "mandala.est.att.roll.value",  title: "roll" }
                ]},
                { name: "P",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.att.pitch.value", title: "pitch cmd" },
                    { bind: "mandala.est.att.pitch.value", title: "pitch" }
                ]},
                { name: "Y",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.pos.bearing.value", title: "bearing cmd" },
                    { bind: "mandala.cmd.att.yaw.value",   title: "yaw cmd" },
                    { bind: "mandala.est.att.yaw.value",   title: "yaw" }
                ]},
                { name: "Axy", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.acc.x.value", title: "Ax" },
                    { bind: "mandala.est.acc.y.value", title: "Ay" }
                ]},
                { name: "Az",  pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.acc.z.value", title: "Az" }
                ]},
                { name: "G",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.gyro.x.value", title: "Gx" },
                    { bind: "mandala.est.gyro.y.value", title: "Gy" },
                    { bind: "mandala.est.gyro.z.value", title: "Gz" }
                ]},
                { name: "Pt",  pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.pos.altitude.value",  title: "alt" },
                    { bind: "mandala.est.pos.vspeed.value",    title: "vspd" },
                    { bind: "mandala.est.air.airspeed.value",  title: "airspeed" }
                ]},
                { name: "Ctr", pin: false, speed: 1.0, items: [
                    { bind: "mandala.ctr.att.ail.value",  title: "ail" },
                    { bind: "mandala.ctr.att.elv.value",  title: "elv" },
                    { bind: "mandala.ctr.att.rud.value",  title: "rud" },
                    { bind: "mandala.ctr.eng.thr.value",  title: "thr" },
                    { bind: "mandala.ctr.eng.prop.value", title: "prop" },
                    { bind: "mandala.ctr.str.rud.value",  title: "str.rud" }
                ]},
                { name: "RC",  pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.rc.roll.value",  title: "RC roll" },
                    { bind: "mandala.cmd.rc.pitch.value", title: "RC pitch" },
                    { bind: "mandala.cmd.rc.thr.value",   title: "RC thr" },
                    { bind: "mandala.cmd.rc.yaw.value",   title: "RC yaw" }
                ]},
                { name: "Usr", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.usr.u1.value", title: "u1" },
                    { bind: "mandala.est.usr.u2.value", title: "u2" },
                    { bind: "mandala.est.usr.u3.value", title: "u3" },
                    { bind: "mandala.est.usr.u4.value", title: "u4" },
                    { bind: "mandala.est.usr.u5.value", title: "u5" },
                    { bind: "mandala.est.usr.u6.value", title: "u6" }
                ]}
            ]
        };
    }
}
/*
 * APX Autopilot project <http://docs.uavos.com\>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/\>.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Common

// Root widget — usable as signalsWidget from child QML files.
Rectangle {
    id: signalsWidget

    implicitHeight: mainLayout.implicitHeight
    implicitWidth: mainLayout.implicitWidth
    border.width: 0
    color: "#000"

    // Active set — a MenuSet Fact instance, rebuilt when set changes
    property var activeSet: null
    // Ordered pages from the active set
    property var activePages: []
    // Currently selected page (for the single-view tab slot)
    property var currentPage: null

    // -----------------------------------------------------------------
    // Public API used by child components
    // -----------------------------------------------------------------

    function saveSettings() {
        var fjson = application.prefs.loadFile("signals.json");
        var json = fjson ? JSON.parse(fjson) : {};
        if (!json.active) json.active = {};
        json.sets = [];
        var activeIdx = 0;
        for (var i = 0; i < setsFact.size; ++i) {
            var sf = setsFact.child(i);
            json.sets.push(sf.save());
            if (sf.active) activeIdx = i;
        }
        json.active["signals"] = activeIdx;
        application.prefs.saveFile("signals.json", JSON.stringify(json, ' ', 2));
    }

    function checkScrMatches(val) {
        if (!activeSet) return false;
        return activeSet.checkScrs(val);
    }

    function activatePage(page) {
        if (currentPage === page) return;
        currentPage = page;
        // Update chart view
        singleChart.resetEnable = true;
        singleChart.facts = Qt.binding(function() { return currentPage ? currentPage.values : []; });
        singleChart.speedFactorValue = page ? page.speed : 1.0;
    }

    function updateLayout() {
        // Rebuild page buttons and pinned charts
        rebuildPageButtons();
        rebuildPinnedCharts();
    }

    function updateSeriesColors() {
        singleChart.updateSeriesColor();
        for (var i = 0; i < pinnedChartsRepeater.count; ++i) {
            var item = pinnedChartsRepeater.itemAt(i);
            if (item && item.chartView)
                item.chartView.updateSeriesColor();
        }
    }

    // -----------------------------------------------------------------
    // Load/rebuild sets from signals.json
    // -----------------------------------------------------------------

    Component.onCompleted: {
        loadSettings();
    }

    function loadSettings() {
        // Destroy old set facts
        setsFact.deleteChildren();

        var f = application.prefs.loadFile("signals.json");
        var json = f ? JSON.parse(f) : {};

        // Legacy migration
        if (json && json.signalas && !json.sets)
            json = migrateLegacy(json);

        var sets = [];
        var activeIdx = 0;

        if (json && json.sets) {
            for (var i in json.sets) {
                var s = json.sets[i];
                if (s && s.pages) sets.push(s);
            }
            if (json.active)
                activeIdx = json.active["signals"] || 0;
        }

        if (sets.length === 0)
            sets.push(buildDefaultSet());

        if (activeIdx < 0 || activeIdx >= sets.length)
            activeIdx = 0;

        // Create MenuSet facts
        for (var i in sets) {
            var ms = createMenuSet(sets[i]);
            ms.active = (parseInt(i) === activeIdx);
        }

        rebuildFromActiveSet();
    }

    function createMenuSet(setData) {
        var component = Qt.createComponent("MenuSet.qml");
        if (component.status !== Component.Ready) {
            console.warn("Signals: cannot load MenuSet.qml: " + component.errorString());
            return null;
        }
        var ms = component.createObject(setsFact, {
            "title": setData.title || "set",
            "pages": setData.pages || []
        });
        ms.parentFact = setsFact;
        return ms;
    }

    // Find the active set and rebuild pages/buttons/charts
    function rebuildFromActiveSet() {
        activeSet = null;
        for (var i = 0; i < setsFact.size; ++i) {
            var sf = setsFact.child(i);
            if (sf.active) { activeSet = sf; break; }
        }
        if (!activeSet && setsFact.size > 0)
            activeSet = setsFact.child(0);

        activePages = activeSet ? activeSet.getPages() : [];
        currentPage = activePages.length > 0 ? activePages[0] : null;

        rebuildPageButtons();
        rebuildPinnedCharts();

        if (currentPage) {
            singleChart.resetEnable = true;
            singleChart.facts = Qt.binding(function() { return currentPage ? currentPage.values : []; });
            singleChart.speedFactorValue = currentPage.speed;
        } else {
            singleChart.facts = [];
        }
    }

    // -----------------------------------------------------------------
    // Page buttons
    // -----------------------------------------------------------------

    property var pageButtons: []

    function rebuildPageButtons() {
        // Destroy old
        for (var i = 0; i < pageButtons.length; ++i)
            pageButtons[i].destroy();
        pageButtons = [];

        if (!activePages) return;
        var btns = [];
        for (var i = 0; i < activePages.length; ++i) {
            var pg = activePages[i];
            var btn = pageBtnComponent.createObject(bottomBar, {
                "page": pg,
                "text": Qt.binding(function() { return pg.title; })
            });
            btn.page = pg;
            btns.push(btn);
        }
        pageButtons = btns;
        // Select first
        if (pageButtons.length > 0)
            pageButtonGroup.checkedButton = pageButtons[0];
    }

    // -----------------------------------------------------------------
    // Pinned charts (stacked)
    // -----------------------------------------------------------------

    property var pinnedPages: []

    function rebuildPinnedCharts() {
        pinnedPages = activePages.filter(function(p) { return p.pinned; });
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
    // Fact container for MenuSet instances (not shown in menu UI here)
    // -----------------------------------------------------------------

    import APX.Facts

    property var setsFact: _setsFact

    // We can't embed a Fact as a property in Rectangle with APX.Facts import
    // so use a loader trick — see _setsFact defined below.

    // -----------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0

        // Pinned pages (stacked, only shown when at least one page is pinned)
        Repeater {
            id: pinnedChartsRepeater
            model: pinnedPages

            delegate: Item {
                property alias chartView: _pinnedChart
                Layout.fillWidth: true
                Layout.preferredHeight: 80 * ui.scale
                Layout.minimumHeight: 20

                ChartsView {
                    id: _pinnedChart
                    anchors.fill: parent
                    facts: modelData.values
                    speedFactorValue: modelData.speed
                }
            }
        }

        // Single (non-pinned) chart view for the active tab
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

        // Bottom bar: page tabs + set label + speed button
        RowLayout {
            id: bottomBar
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24 * ui.scale

            // Page tab buttons are added here dynamically via rebuildPageButtons()
            // (they are children of bottomBar and part of pageButtonGroup)

            Item { Layout.fillWidth: true }  // spacer

            // Set name label — click opens sets editor
            TextButton {
                id: setLabel
                text: activeSet ? activeSet.title : ""
                Layout.fillHeight: true
                Layout.minimumWidth: height * 3
                toolTip: qsTr("Click to edit chart sets")
                onClicked: openSetsEditor()
            }

            // Speed button — cycles per-page speed
            TextButton {
                id: speedBtn
                text: currentPage ? (currentPage.speed + "x") : "1x"
                Layout.fillHeight: true
                Layout.minimumWidth: height * 3
                onClicked: {
                    if (!currentPage) return;
                    var factors = [0.2, 0.5, 1, 2, 4];
                    var idx = factors.indexOf(currentPage.speed);
                    var next = (idx >= 0 && idx < factors.length - 1) ? factors[idx + 1] : factors[0];
                    currentPage.speed = next;
                    singleChart.speedFactorValue = next;
                    saveSettings();
                }
            }
        }
    }

    // + button (top-right) — opens active page item editor
    IconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Style.spacing
        size: Style.buttonSize * 0.7
        iconName: "plus"
        toolTip: qsTr("Edit current page items")
        opacity: ui.effects ? (hovered ? 1 : 0.5) : 1
        onTriggered: {
            if (currentPage)
                currentPage.addNewItem();
        }
    }

    // -----------------------------------------------------------------
    // Sets editor popup
    // -----------------------------------------------------------------

    function openSetsEditor() {
        if (setsEditorPopup) return;
        var c = Qt.createComponent("SignalsMenuPopup.qml", Component.PreferSynchronous, ui.window);
        if (c.status === Component.Ready) {
            var obj = c.createObject(ui.window);
            setsEditorPopup = obj;
            obj.accepted.connect(function() {
                loadSettings();
            });
            obj.closed.connect(function() {
                setsEditorPopup = null;
            });
            obj.open();
        } else {
            console.warn("Signals: cannot open sets editor: " + c.errorString());
        }
    }

    property var setsEditorPopup: null

    // -----------------------------------------------------------------
    // Legacy migration helper
    // -----------------------------------------------------------------

    function migrateLegacy(oldJson) {
        var items = (oldJson.signalas || []).map(function(it) {
            return {
                bind: it.bind || it.name || "",
                title: it.title || "",
                color: it.color || "",
                filters: [],
                warn: it.warn || "",
                alarm: it.alarm || "",
                act: it.act || "",
                save: it.save || ""
            };
        }).filter(function(it) { return it.bind !== ""; });

        return {
            active: { signals: 0 },
            sets: [{
                title: "default",
                pages: [{
                    name: oldJson.page || "page 1",
                    pin: false,
                    speed: 1.0,
                    items: items
                }]
            }]
        };
    }

    // -----------------------------------------------------------------
    // Default set factory
    // -----------------------------------------------------------------

    function buildDefaultSet() {
        return {
            title: "default",
            pages: [
                { name: "R",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.att.roll.value",  title: "roll cmd" },
                    { bind: "mandala.est.att.roll.value",  title: "roll" }
                ]},
                { name: "P",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.att.pitch.value", title: "pitch cmd" },
                    { bind: "mandala.est.att.pitch.value", title: "pitch" }
                ]},
                { name: "Y",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.pos.bearing.value", title: "bearing cmd" },
                    { bind: "mandala.cmd.att.yaw.value",   title: "yaw cmd" },
                    { bind: "mandala.est.att.yaw.value",   title: "yaw" }
                ]},
                { name: "Axy", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.acc.x.value", title: "Ax" },
                    { bind: "mandala.est.acc.y.value", title: "Ay" }
                ]},
                { name: "Az",  pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.acc.z.value", title: "Az" }
                ]},
                { name: "G",   pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.gyro.x.value", title: "Gx" },
                    { bind: "mandala.est.gyro.y.value", title: "Gy" },
                    { bind: "mandala.est.gyro.z.value", title: "Gz" }
                ]},
                { name: "Pt",  pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.pos.altitude.value",  title: "alt" },
                    { bind: "mandala.est.pos.vspeed.value",    title: "vspd" },
                    { bind: "mandala.est.air.airspeed.value",  title: "airspeed" }
                ]},
                { name: "Ctr", pin: false, speed: 1.0, items: [
                    { bind: "mandala.ctr.att.ail.value",  title: "ail" },
                    { bind: "mandala.ctr.att.elv.value",  title: "elv" },
                    { bind: "mandala.ctr.att.rud.value",  title: "rud" },
                    { bind: "mandala.ctr.eng.thr.value",  title: "thr" },
                    { bind: "mandala.ctr.eng.prop.value", title: "prop" },
                    { bind: "mandala.ctr.str.rud.value",  title: "str.rud" }
                ]},
                { name: "RC",  pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.rc.roll.value",  title: "RC roll" },
                    { bind: "mandala.cmd.rc.pitch.value", title: "RC pitch" },
                    { bind: "mandala.cmd.rc.thr.value",   title: "RC thr" },
                    { bind: "mandala.cmd.rc.yaw.value",   title: "RC yaw" }
                ]},
                { name: "Usr", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.usr.u1.value", title: "u1" },
                    { bind: "mandala.est.usr.u2.value", title: "u2" },
                    { bind: "mandala.est.usr.u3.value", title: "u3" },
                    { bind: "mandala.est.usr.u4.value", title: "u4" },
                    { bind: "mandala.est.usr.u5.value", title: "u5" },
                    { bind: "mandala.est.usr.u6.value", title: "u6" }
                ]}
            ]
        };
    }

    // Component for dynamic page buttons
    Component {
        id: pageBtnComponent
        PageButton {
            ButtonGroup.group: pageButtonGroup
        }
    }
}
