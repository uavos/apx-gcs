import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import APX.Facts 1.0
import Apx.Common 1.0

ToolButton {
    id: control

    property QtObject fact

    property bool noFactTrigger: false

    property int treeType: fact?fact.treeType:Fact.NoFlags
    property int dataType: fact?fact.dataType:Fact.NoFlags
    property int options: fact?fact.options:Fact.NoFlags

    property string descr: fact?fact.descr:""

    iconName: fact?fact.icon:""
    text: fact?fact.title:""

    enabled: fact?fact.enabled:true
    visible: (fact?fact.visible:true) && (bShowDisabled || enabled)

    property bool active: fact?fact.active:false
    highlighted: activeFocus || active

    showText: !bIconOnly

    toolTip: descr


    //internal
    property bool bApply: dataType==Fact.Apply
    property bool bRemove: dataType==Fact.Remove
    property bool bStop: dataType==Fact.Stop

    property bool bIconOnly: options&Fact.IconOnly
    property bool bShowDisabled: options&Fact.ShowDisabled

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

    onTriggered: if(fact && !noFactTrigger) fact.trigger()
}
