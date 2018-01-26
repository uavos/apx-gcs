import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

FactMenuEditorInt {
    id: control

    property int decimals: Math.min(2,fact.precision>0?fact.precision:2)
    //onDecimalsChanged: console.log(decimals)

    div: Math.pow(10,decimals)

    validator: DoubleValidator {
        bottom: Math.min(control.from, control.to)
        top:  Math.max(control.from, control.to)
    }

    textFromValue: function(value, locale) {
        return textWithUnits(fact.text)
    }

    valueFromText: function(text, locale) {
        return Number.fromLocaleString(locale, text) * div
    }
}
