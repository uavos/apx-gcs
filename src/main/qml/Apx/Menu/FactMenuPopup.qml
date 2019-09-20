import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2

import APX.Facts 1.0

import Apx.Common 1.0
import Apx.Menu 1.0

Popup {
    id: popupItem

    property point pos: Qt.point(parent.width/2,parent.height/2)

    //forward props
    property alias fact: factMenu.fact
    property alias currentFact: factMenu.currentFact
    property alias showTitle: factMenu.showTitle

    property bool pinned: false

    onClosed: {
        if(typeof(destroy)!=='undefined')destroy()
    }
    onFactChanged: if(!fact)close()

    x: pos.x - width/2
    y: py
    readonly property int py: pos.y// - height/2
    onPyChanged: {
        //y=Qt.binding(function(){return py<0?py:Math.min(y,py)})
        //console.log(y,height)
    }

    /*onAboutToShow: {
        x=pos.x - width/2
        y=pos.y - height/2
    }*/

    padding: 0
    margins: 0

    //Material.background: "black"
    //opacity: 1

    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: ui.smooth?0.9:1 } }

    closePolicy: pinned?Popup.NoAutoClose:(Popup.CloseOnEscape|Popup.CloseOnPressOutside)

    contentItem: FactMenu {
        id: factMenu
        autoResize: true
        //onFactRemoved: popupItem.close()
        //onFactTriggered: if(closeOnTrigger)popupItem.close()
        onFactTriggered: {
            if(fact.options & Fact.CloseOnTrigger)
                popupItem.close()
        }
        onStackEmpty: popupItem.close()
        titleRightMargin: btnClose.width
        CleanButton {
            id: btnClose
            z: 10
            anchors.right: factMenu.right
            anchors.top: factMenu.top
            anchors.margins: 5
            iconName: "close"
            color: popupItem.pinned?Material.BlueGrey:undefined
            height: MenuStyle.titleSize*0.8
            width: height
            onClicked: popupItem.close()
        }
    }
    MouseArea {
        anchors.fill: parent
        property point clickPos: Qt.point(0,0)
        onPressed: clickPos = Qt.point(mouse.x,mouse.y)
        onPositionChanged: {
            var delta = Qt.point(mouse.x-clickPos.x, mouse.y-clickPos.y)
            //popupItem.pos = Qt.point(popupItem.pos.x+delta.x,popupItem.pos.y+delta.y)
            popupItem.x+=delta.x
            popupItem.y+=delta.y
            popupItem.pinned=true
            //console.log(mouse.x-clickPos.x,mouse.y-clickPos.y)
        }
        onDoubleClicked: popupItem.pinned=true
        onPressAndHold: popupItem.pinned=true
    }
}
