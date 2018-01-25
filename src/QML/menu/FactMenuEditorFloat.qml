import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

FactMenuEditorInt {
    id: spinbox

    property int decimals: Math.min(2,fact.precision>0?fact.precision:2)
    onDecimalsChanged: console.log(decimals)

    div: Math.pow(10,decimals)

    validator: DoubleValidator {
        bottom: Math.min(spinbox.from, spinbox.to)
        top:  Math.max(spinbox.from, spinbox.to)
    }

    textFromValue: function(value, locale) {
        return fact.text //Number(value / div).toLocaleString(locale, 'f', spinbox.decimals)
    }

    valueFromText: function(text, locale) {
        return Number.fromLocaleString(locale, text) * div
    }
}
