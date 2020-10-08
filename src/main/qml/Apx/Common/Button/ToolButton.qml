import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import ".."

IconButton {
    id: control

    contentComponent: Component {
        id: _toolC
        Row {
            id: _row
            spacing: 2
            Loader {
                active: control.showIcon
                height: parent.height
                sourceComponent: control.iconC
            }
            Loader {
                active: control.showText
                height: parent.height
                sourceComponent: control.textC
            }
        }
    }
    property Component toolC: _toolC
}
