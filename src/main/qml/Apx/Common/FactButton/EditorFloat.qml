import QtQuick 2.6
import QtQuick.Controls 2.2

EditorInt {
    id: editor

    property int decimals: Math.min(2,fact.precision>0?fact.precision:2)

    div: Math.pow(10,decimals)

    validator: DoubleValidator {
        bottom: Math.min(editor.from, editor.to)
        top:  Math.max(editor.from, editor.to)
    }

    textFromValue: function(value, locale) {
        return textWithUnits(fact.text)
    }

    valueFromText: function(text, locale) {
        return Number.fromLocaleString(locale, text) * div
    }
}
