import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQml 2.12
import QtQuick.Layouts 1.3

//import ".."

ComboBox {
    id: editor

    spacing: 0
    topPadding: 0
    bottomPadding: 0
    background.y: 0
    background.height: height
    //contentItem.height: height

    //Layout.fillHeight: true
    implicitHeight: bodyHeight

    topInset: 0
    bottomInset: 0

    font.family: font_condenced
    font.pixelSize: fontSize(bodyHeight*valueSize)

    contentItem.implicitWidth: contentItem.contentWidth+indicator.width/2 //+editor.height/2

    model: fact.enumStrings

    Component.onCompleted: updateIndex()
    onActivated: {
        fact.setValue(textAt(index))
        parent.forceActiveFocus();
    }
    property string value: fact.text
    onValueChanged: updateIndex()
    onModelChanged: updateIndex()

    Connections {
        target: listView
        onMovementStarted: {
            editor.popup.close()
        }
    }
    popup.width: factButton.width*0.7
    popup.x: editor.width-popup.width
    function updateIndex()
    {
        //currentIndex=find(value)
        editor.currentIndex=fact.enumStrings.indexOf(editor.value)
        //console.log(currentIndex,value,count,find(value))
    }

    //BoundingRect {}
}
