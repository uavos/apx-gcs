import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

Row{
    spacing: editorFontSize/2
    Text {
        text: fact.text
        color: Style.cValueConst
        font.family: font_condenced
        font.pixelSize: editorFontSize
    }
    Text {
        visible: fact.units
        text: visible?"["+fact.units+"]":""
        color: Style.cTextDisabled
        font.family: font_condenced
        font.pixelSize: editorFontSize
    }
}
