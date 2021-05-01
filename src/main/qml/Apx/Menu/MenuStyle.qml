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
pragma Singleton
import QtQuick 2.9
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

QtObject {

    readonly property real itemSize: Style.buttonSize
    readonly property real itemWidth: itemSize*Style.widthRatio

    readonly property color cBg:             "#C0000000"
    readonly property color cBgField:        "#80303030"
    readonly property color cBgHover:        "#40f0f0f0"
    readonly property color cBgPress:        "#5530FF60"


    readonly property color cBgListItem:     "#80303030"
    readonly property color cBgListItemSel:  Material.color(Material.BlueGrey)
    readonly property color cText:           "#fff"
    readonly property color cTextDisabled:   "#aaa"
    readonly property color cTextActive:     "#3f6"
    readonly property color cTextModified:   "#ff6"

    readonly property color cTitleSep:       "#5c8fff"
    readonly property color cSep:            "#667"

    readonly property color cValueConst:     "#aaa"
    readonly property color cValueText:      "#fff"
    readonly property color cValueTextEdit:  "#FFFF60"
    readonly property color cValueFrame:     "#a0FFe0"

    readonly property color cActionRemove:   "#a55"
    readonly property color cActionStop:     "#a55"
    readonly property color cActionApply:    "#2a4"
    readonly property color cStatusText:     "#aaa"
}
