import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2
import GCS.FactSystem 1.0

import QtQuick.Templates 2.3 as T
import QtQuick.Controls.impl 2.3


import "."

Button {
    id: factButton
    property var fact


    //internal
    property bool expandable: fact.size || fact.treeItemType===Fact.GroupItem // || bEnumChilds
    property bool isAction: fact.dataType===Fact.ActionData

    property Item editor

    onClicked: if(activeFocus){
        if(expandable) openFact(fact)
        else if(isAction)actionTriggered(fact)
        //else if(bEnumChilds) openFact(fact)
        fact.trigger()
        factTriggered(fact)
        //else if(isAction && fact.value!==Fact.RemoveAction) back();
    }

    onPressAndHold: {
        //listProperty(fact)
        openFact(fact,{"pageInfo": true})
    }

    Connections {
        target: fact
        onRemoved: {
            fact=dummyFactC.createObject(menuPage);
            destroy()
        }
    }
    Component {
        id: dummyFactC
        FactMenuElement { }
    }

    visible: fact.visible && (!isActionFact(fact))

    enabled: fact.enabled
    hoverEnabled: enabled

    flat: true
    highlighted: fact.active

    implicitWidth: parent.width
    implicitHeight: visible?itemSize:0

    padding: 3
    leftPadding: padding+1
    rightPadding: padding
    topPadding: padding
    bottomPadding: padding
    spacing: 0

    //background.implicitHeight: itemSize //contentItem.implicitHeight

    background.y: 0
    background.width: width
    background.height: height-1
    Material.background: Style.cBgListItem

    text: fact.title

    contentItem: RowLayout{
        spacing: 4
        //fact icon
        Text {
            Layout.rightMargin: 4
            visible: fact.iconSource
            font.family: "Material Design Icons"
            font.pointSize: iconFontSize
            text: visible?materialIconChar[fact.iconSource]:""
            color: !factButton.enabled ? Style.cTextDisabled : Style.cText
        }
        //fact title & descr
        Item {
            id: titleItem
            Layout.fillHeight: true
            Layout.fillWidth: true
            //Layout.minimumWidth: titleText.contentWidth/2
            //Layout.preferredWidth: titleText.contentWidth
            Text {
                id: titleText
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                //anchors.topMargin: -3
                verticalAlignment: descrText.visible?Text.AlignTop:Text.AlignVCenter
                text: factButton.text
                font.pointSize: titleFontSize
                color: !factButton.enabled ? Style.cTextDisabled :
                    factButton.highlighted ? Style.cTextActive :
                    fact.modified ? Style.cTextModified : Style.cText
            }
            Text {
                id: descrText
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                text: fact.descr
                font.pointSize: descrFontSize
                color: Style.cTextDisabled
                visible: text!==''
            }
        }

        //status text
        Text {
            text: fact.status
            font.pointSize: editorFontSize
            color: Style.cTextDisabled
        }

        //editor
        RowLayout {
            id: editorContainer
            property var editor
            Layout.fillHeight: true
            Layout.fillWidth: editor?true:false
            Component.onCompleted: {
                if(fact.treeItemType === Fact.FactItem){
                    editor=createEditor(editorContainer)
                }
            }
        }


        //next icon
        Text {
            visible: expandable
            Layout.maximumWidth: font.pointSize*0.7
            Layout.leftMargin: -font.pointSize*0.25
            font.family: "Material Design Icons"
            font.pointSize: itemSize*0.8
            text: visible?materialIconChar["chevron-right"]:""
            color: !factButton.enabled ? Style.cTextDisabled : Style.cText
        }

    }

    function createEditor(parentItem)
    {
        var qml=""
        switch(fact.dataType){
        case Fact.BoolData:
            qml="Switch"
            break
        case Fact.TextData:
            if(fact.enumStrings.length > 0)qml="TextOption"
            else qml="Text"
            break
        case Fact.EnumData:
            qml="Option"
            break
        case Fact.KeySequenceData:
            qml="Key"
            break
        case Fact.IntData:
            if(fact.units==="time")qml="Time"
            else qml="Int"
            break
        case Fact.FloatData:
            if(fact.units==="lat" || fact.units==="lon")qml="Text"
            else qml="Float"
            break

        default:
            qml="Const"
        }


        //load and create component
        var opts={
            "Layout.fillHeight": true,
            "Layout.alignment": Qt.AlignVCenter|Qt.AlignRight,
            "anchors.verticalCenter": parentItem.verticalCenter
        }

        var cmp=Qt.createComponent("FactMenuEditor"+qml+".qml",parentItem)
        if (cmp.status === Component.Ready) {
            return cmp.createObject(parentItem,opts);
        }
    }


    Component {
        id: testC

        Rectangle {
            Layout.alignment: Qt.AlignRight
            implicitHeight: 20
            implicitWidth: 20
            border.width: 1
            border.color: "green"
            color: 'gray'
        }
    }

}


