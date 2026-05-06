import QtQuick


import APX.Facts

Fact {
    id: colorFact

    property string colorValue: ""

    flags: Fact.CloseOnTrigger
    opts: ({
               "editor": Qt.resolvedUrl("SignalsColorEditor.qml")
    })

    onTriggered: setColor()

    function setColor() {
        if(!parentFact)
            return;
        if(title === qsTr("Auto")) {  
            setAutoColor()
            return;
        }
        parentFact.value = colorValue; 
    }

    function setAutoColor() {
        if (title !== qsTr("Auto"))
            return;
        if (!parentFact)
            return;
        parentFact.setDefaultColor()
    }
}

