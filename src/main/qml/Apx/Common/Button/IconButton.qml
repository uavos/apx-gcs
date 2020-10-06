import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import ".."

TextButton {
    id: control

    property string iconName
    property bool showIcon: true

    property color iconColor: Material.iconColor
    property color disabledIconColor: Material.iconDisabledColor

    readonly property real iconScale: 0.9
    readonly property int iconSize: Math.max(7, control.height * iconScale - 2)

    property color currentIconColor: enabled?iconColor:disabledIconColor

    readonly property Item iconItem: MaterialIcon {
        visible: showIcon && iconName
        size: iconSize
        color: currentIconColor
        name: control.iconName

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    contentItem: iconItem
}
