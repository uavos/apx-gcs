import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

SpinBox {
    id: editor
    from: (typeof fact.min!=='undefined')?fact.min*div:-1000000000
    to: (typeof fact.max!=='undefined')?fact.max*div:1000000000
    value: fact.value*div

    property int wgrow: implicitWidth
    onImplicitWidthChanged: {
        if(implicitWidth<wgrow)implicitWidth=wgrow
        else wgrow=implicitWidth
    }

    stepSize: (fact.units==="m" && div==1 && (value>=10))?10:1

    font.family: font_condenced
    font.pixelSize: control.valueSize

    up.onPressedChanged: if(activeFocus)editor.parent.forceActiveFocus()
    down.onPressedChanged: if(activeFocus)editor.parent.forceActiveFocus()
    contentItem: Item{
        implicitWidth: textInput.contentWidth
        TextInput {
            id: textInput
            anchors.centerIn: parent
            font: editor.font

            color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
            text: fact.text

            activeFocusOnTab: true

            selectByMouse: true
            onEditingFinished: {
                fact.setValue(text);
                control.focusFree();
            }
            onActiveFocusChanged: {
                if(activeFocus){
                    if(fact.units === "hex") text=fact.value.toString(16).toUpperCase()
                    else text=fact.value
                    selectAll();
                }else{
                    text=Qt.binding(function(){return fact.text})
                }
            }
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            Rectangle {
                z: parent.z-1
                visible: fact.enabled
                anchors.centerIn: parent
                width: parent.width+10
                height: parent.height
                radius: 3
                color: "#000"
                border.width: 0
                opacity: 0.3
            }
        }
    }


    spacing: 0
    topPadding: 0
    bottomPadding: 0
    baselineOffset: 0

    background: Item {
        implicitWidth: editor.height*3
    }

    leftPadding: 0
    rightPadding: 0


    property real div: 1

    onValueModified: {
        var s=value/div
        if(fact.units === "hex") s=s.toString(16)

        fact.setValue(s)
        value=Qt.binding(function(){return Math.round(fact.value*div)})
    }

}
