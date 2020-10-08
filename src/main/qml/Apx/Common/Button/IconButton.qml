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

    property real iconScale: 0.9
    property int iconSize: Math.max(7, control.height * iconScale - 2)

    property color currentIconColor: enabled?iconColor:disabledIconColor

    contentComponent: Component {
        id: _iconC
        MaterialIcon {
            visible: showIcon && iconName
            size: iconSize
            color: currentIconColor
            name: control.iconName

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    property Component iconC: _iconC
}
