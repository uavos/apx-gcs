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
import QtQuick.Controls
import QtQuick.Layouts

import APX.Facts

import Apx.Common
import Apx.Menu

Item {
    id: root

    property var pageFact: fact ? fact : null
    property var listFact: pageFact ? pageFact.pagesFact : null

    function triggerFact(itemFact) {
        if (!itemFact)
            return;
        if (itemFact.treeType === Fact.Action || itemFact.dataType === Fact.Apply || itemFact.dataType === Fact.Remove || itemFact.dataType === Fact.Stop) {
            itemFact.trigger();
            return;
        }
        menuPage.factButtonTriggered(itemFact);
    }

    clip: true

    ColumnLayout {
        anchors.fill: parent
        spacing: Style.spacing

        FactButton {
            Layout.fillWidth: true
            visible: root.pageFact !== null
            fact: root.pageFact ? root.pageFact.titleFact : null
            noFactTrigger: true
            onTriggered: root.triggerFact(fact)
        }

        FactButton {
            Layout.fillWidth: true
            visible: root.pageFact !== null
            fact: root.pageFact ? root.pageFact.addPageFact : null
            noFactTrigger: true
            onTriggered: root.triggerFact(fact)
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0
            model: root.listFact ? root.listFact.model : null

            delegate: Loader {
                active: modelData ? modelData.visible : false
                visible: active
                width: listView.width
                height: active ? MenuStyle.itemSize : 0
                sourceComponent: Component {
                    FactButton {
                        fact: modelData ? modelData : null
                        noFactTrigger: true
                        size: MenuStyle.itemSize
                        onTriggered: {
                            listView.currentIndex = index;
                            root.triggerFact(fact);
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }
}
