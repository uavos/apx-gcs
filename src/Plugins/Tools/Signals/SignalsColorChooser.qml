import QtQuick


import APX.Facts

Fact {
    id: colorFact

    property string colorAuto: parentFact ? parentFact.colorAuto : "#E57373" 
    property string colorValue: ""

    flags: Fact.CloseOnTrigger
    opts: ({
               "editor": Qt.resolvedUrl("SignalsColorEditor.qml")
    })

    onTriggered: setColor()

    function setColor() {
        if(!parentFact)
            return;
        if(title === colorAuto) {  
            setAutoColor()
            return;
        }
        parentFact.value = colorValue; 
    }

    function setAutoColor() {
        if (title !== colorAuto)
            return;
        if (!parentFact)
            return;
        parentFact.setDefaultColor()
    }
}

