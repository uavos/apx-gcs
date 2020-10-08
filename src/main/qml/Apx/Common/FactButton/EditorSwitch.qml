import QtQuick 2.6
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.12

Switch {
    id: editor

    scale: ui_scale

    implicitWidth: (implicitIndicatorWidth + leftPadding + rightPadding) * scale

    checked: fact.value>0?true:false
    enabled: fact.enabled
    onToggled: {
        if((fact.value>0?true:false)!==checked){
            fact.setValue(checked)
        }
    }
    contentItem: null
}
