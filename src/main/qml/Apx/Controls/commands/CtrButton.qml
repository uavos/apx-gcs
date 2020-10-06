import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

TextButton {
    id: control
    property var fact

    text: fact.title
    toolTip: fact.descr

    size: buttonHeight
}
