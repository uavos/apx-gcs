pragma Singleton
import QtQuick 2.9
import QtQuick.Controls.Material 2.2

QtObject {
    // MenuStyle.

    property int itemSize: 32*ui.scale
    property int titleSize: itemSize
    property int itemWidth: itemSize*widthRatio
    property int widthRatio: 12

    property int titleFontSize: itemSize*0.5
    property int descrFontSize: itemSize*0.35
    property int iconFontSize: itemSize*0.75
    property int editorFontSize: itemSize*0.5

    property color cBg:             "#C0000000"
    property color cBgField:        "#80303030"
    property color cBgHover:        "#40f0f0f0"
    property color cBgPress:        "#5530FF60"


    property color cBgListItem:     "#80303030"
    property color cBgListItemSel:  Material.color(Material.BlueGrey)
    property color cText:           "#fff"
    property color cTextDisabled:   "#aaa"
    property color cTextActive:     "#3f6"
    property color cTextModified:   "#ff6"

    property color cTitleSep:       "#5c8fff"
    property color cSep:            "#667"

    property color cValueConst:     "#aaa"
    property color cValueText:      "#fff"
    property color cValueTextEdit:  "#FFFF60"
    property color cValueFrame:     "#a0FFe0"

    property color cActionRemove:   "#a55"
    property color cActionStop:     "#a55"
    property color cActionApply:    "#2a4"
    property color cStatusText:     "#aaa"
}
