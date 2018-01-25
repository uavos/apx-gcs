import QtQuick 2.6
import QtQuick.Controls 2.2
import GCS.FactSystem 1.0

Switch {
    checked: fact.value
    enabled: fact.enabled
    onClicked: fact.setValue(fact.value?false:true)
}
