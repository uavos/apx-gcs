import QtQuick 2.6
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.2

Switch {
    checked: fact.value?true:false
    enabled: fact.enabled
    onToggled: {
        if(fact.value!==checked){
            fact.setValue(checked)
        }
    }
}
