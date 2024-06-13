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

import Apx.Common

ScrollView {
    id: infoItem

    TextArea {
        id: textArea
        // width: parent.width
        selectByMouse: true
        selectByKeyboard: true
        text: fact.opts.info
        textFormat: TextEdit.AutoText
        wrapMode: TextEdit.NoWrap
        font: apx.font_narrow(Style.fontSize)
        topInset: 0
        bottomInset: 0
        leftInset: 0
        rightInset: 0
        topPadding: 3
        bottomPadding: 3
        leftPadding: 3
        rightPadding: 3
    }
}
