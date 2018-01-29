import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0

import "."

SpinBox {
    id: control
    from: (typeof fact.min!=='undefined')?fact.min*div:-1000000000
    to: (typeof fact.max!=='undefined')?fact.max*div:1000000000
    value: fact.value*div

    //property string inputMask
    //wheelEnabled: true

    property int wgrow: implicitWidth
    onImplicitWidthChanged: {
        if(implicitWidth<wgrow)implicitWidth=wgrow
        else wgrow=implicitWidth
    }

    stepSize: (fact.units==="m" && div==1 && (value>=10))?10:1

    //font.family: font_mono
    //font.bold: true
    font.pointSize: editorFontSize

    focus: true
    //onValueModified: parent.forceActiveFocus();

    up.onPressedChanged: if(activeFocus)control.parent.forceActiveFocus()
    down.onPressedChanged: if(activeFocus)control.parent.forceActiveFocus()
    /*contentItem: Text {
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: enabled ? Style.cText : Style.cTextDisabled
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }*/
    contentItem: Item{
        TextInput {
            anchors.centerIn: parent
            font.family: font_condenced
            font.pixelSize: editorFontSize

            //inputMask: control.inputMask

            color: activeFocus?Style.cValueTextEdit:Style.cValueText
            text: textWithUnits(fact.text)

            /*Connections {
                target: fact
                onValueChanged: if(activeFocus)control.parent.forceActiveFocus();
            }*/

            selectByMouse: true
            onEditingFinished: {
                fact.setValue(text);
                //control.parent.forceActiveFocus();
            }
            onActiveFocusChanged: {
                if(activeFocus){
                    text=fact.text
                    selectAll();
                }else{
                    text=Qt.binding(function(){return textWithUnits(fact.text)})
                }
            }
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            Rectangle {
                z: parent.z-1
                visible: fact.enabled
                anchors.centerIn: parent
                width: parent.width+10
                height: editorFontSize+10
                radius: 3
                color: "#000"
                border.width: 0
                border.color: Style.cValueFrame
                opacity: 0.3
            }
        }
    }


    spacing: 0
    topPadding: 0
    bottomPadding: 0
    baselineOffset: 0

    background: Item {
        implicitWidth: itemSize*3.5
    }
    //background.implicitWidth: itemSize*3.5

    leftPadding: 0
    rightPadding: 0



    property real div: 1

    /*textFromValue: function(value) {
        var i=(value/div).toFixed();
        if(i>=0 && i<fact.enumStrings.length){
            return fact.enumStrings[i]
        }
        return textWithUnits(i);
    }*/
    function textWithUnits(s)
    {
        var u=fact.units
        if(!u) return s
        if(u.indexOf("..")>=0)return s
        if(isNaN(s))return s
        //if(s !== parseFloat(s))return s
        return s+" "+u
    }

    onValueModified: {
        fact.setValue(value/div)
        value=Qt.binding(function(){return Math.round(fact.value*div)})
    }

    FactMenuEditorDialog { }

}
