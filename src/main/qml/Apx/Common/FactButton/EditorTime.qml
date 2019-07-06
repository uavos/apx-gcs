import QtQuick 2.6
import QtQuick.Controls 2.2

EditorInt {
    id: control

    textFromValue: function(value, locale) {
        return fact.text
    }

    valueFromText: function(text, locale) {
        return apx.timeFromString(text)
    }
}
