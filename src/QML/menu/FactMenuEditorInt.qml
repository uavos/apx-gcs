import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

SpinBox {
    from: (typeof fact.min!=='undefined')?fact.min*div:-1000000000
    to: (typeof fact.max!=='undefined')?fact.max*div:1000000000
    value: fact.value*div

    stepSize: fact.units==="m"?10*div:1

    font.pointSize: editorFontSize

    property real div: 1

    textFromValue: function(value) {
        var i=(value/div).toFixed();
        if(i>=0 && i<fact.enumStrings.length){
            return fact.enumStrings[i]
        }
        if(fact.units) return i+" "+fact.units
        return i;
    }

    onValueModified: {
        fact.setValue(value/div)
        value=Qt.binding(function(){return Math.round(fact.value*div)})
    }

    FactMenuEditorDialog { }

    wheelEnabled: true
}
