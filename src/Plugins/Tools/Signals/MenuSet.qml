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

import APX.Facts

// One set — a named collection of pages.
// Mirrors NumbersMenuSet but contains pages (MenuPage) instead of values.
Fact {
    id: setFact

    flags: (Fact.Group | Fact.FlatModel)

    property var pages: [] // from config / JSON
    property var titleFact: setTitle
    property var addPageFact: addPage
    property var pagesFact: mPages

    signal selected(var num)

    Fact {
        id: setTitle
        title: qsTr("Description")
        flags: Fact.Text
        icon: "rename-box"
        value: setFact.title
        onValueChanged: setFact.title = value
    }

    // Add a new blank page (up to 10)
    Fact {
        id: addPage
        title: qsTr("Add page")
        icon: "plus-circle"
        flags: Fact.Action
        enabled: mPages.size < 10
        onTriggered: {
            var n = mPages.size + 1;
            var pageData = { name: "P" + n, pin: false, speed: 1.0, items: [] };
            createPage(pageData);
        }
    }

    Fact {
        id: mPages
        title: qsTr("Pages")
        flags: (Fact.Group | Fact.Section | Fact.Count | Fact.DragChildren)
        onSizeChanged: updateDescr()
        onItemMoved: updateDescr()
    }

    Component.onCompleted: {
        var opt = opts;
        opt.page = "qrc:/Signals/MenuSetPage.qml";
        opts = opt;
        updateSetItems();
    }

    function save() {
        var savedPages = [];
        for (var i = 0; i < mPages.size; ++i) {
            var pg = mPages.child(i);
            savedPages.push(pg.save());
        }
        return {
            title: title,
            pages: savedPages
        };
    }

    function loadSet(setData) {
        title = setData.title ? setData.title : title;
        setTitle.value = title;
        pages = setData.pages ? setData.pages : [];
        updateSetItems();
    }

    function updateSetItems() {
        mPages.deleteChildren();
        var plist = pages ? pages : [];
        for (var i in plist)
            createPage(plist[i]);
        updateDescr();
    }

    function createPage(pageData) {
        if (mPages.size >= 10)
            return null;
        var component = Qt.createComponent("MenuPage.qml");
        if (component.status !== Component.Ready) {
            console.warn("MenuSet: cannot load MenuPage.qml: " + component.errorString());
            return null;
        }
        var pg = component.createObject(mPages, {
            "title": pageData.name ? pageData.name : ("P" + (mPages.size + 1))
        });
        if (!pg) {
            console.warn("MenuSet: failed to create MenuPage instance");
            return null;
        }
        pg.parentFact = mPages;
        pg.load(pageData);
        if (pg.titleChanged)
            pg.titleChanged.connect(updateDescr);
        updateDescr();
        return pg;
    }

    function getPages() {
        var list = [];
        for (var i = 0; i < mPages.size; ++i)
            list.push(mPages.child(i));
        return list;
    }

    function updateDescr() {
        if (!setFact)
            return;
        var s = [];
        for (var i = 0; i < mPages.size; ++i)
            s.push(mPages.child(i).title);
        descr = s.join(", ");
    }

    function checkScrs(val) {
        var matches = false;
        for (var i = 0; i < mPages.size; ++i)
            if (mPages.child(i).checkScrs(val))
                matches = true;
        return matches;
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove set")
        icon: "delete"
        onTriggered: {
            if (setFact.active)
                selected(0);
            setFact.destroy();
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Select and save")
        visible: !setFact.active
        icon: "check-circle"
        onTriggered: {
            setFact.menuBack();
            setFact.selected(setFact.num);
        }
    }
}
