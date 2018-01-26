import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

FactMenuEditorInt {
    id: control

    //inputMask: fact.text===fact.value?"":"99:99:99"

    //validator: RegExpValidator { regExp: /^([0-1]?[0-9]|2[0-3]):([0-5][0-9]):[0-5][0-9]$ / }
    //background.implicitWidth: itemSize*4

    textFromValue: function(value, locale) {
        return fact.text
    }

    valueFromText: function(text, locale) {
        return app.timeFromString(text)
    }
}
