import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import APX.Facts 1.0
import Apx.Common 1.0

ToolButton {
    id: control

    property QtObject fact

    property int options: fact.options
    property int dataType: fact.dataType

    toolTip: fact.descr
    iconName: fact.icon
    text: fact.title

    enabled: fact.enabled
    visible: fact.visible && (bShowDisabled || enabled)

    property bool active: fact.active
    highlighted: activeFocus || active

    showText: !bIconOnly

    //internal
    readonly property bool bApply: dataType==Fact.Apply
    readonly property bool bRemove: dataType==Fact.Remove
    readonly property bool bStop: dataType==Fact.Stop

    readonly property bool bIconOnly: options&Fact.IconOnly
    readonly property bool bShowDisabled: options&Fact.ShowDisabled

    function action_color()
    {
        switch(dataType){
        case Fact.Apply: return Material.color(Material.Green)
        case Fact.Remove:
        case Fact.Stop: return Material.color(Material.Red)
        }
        return undefined
    }

    color: action_color()

    onTriggered: fact.trigger()
}
