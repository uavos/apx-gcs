import QtQuick 2.6
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.2

Switch {
    checked: fact.value>0?true:false
    enabled: fact.enabled
    onToggled: {
        if((fact.value>0?true:false)!==checked){
            fact.setValue(checked)
        }
    }
}
