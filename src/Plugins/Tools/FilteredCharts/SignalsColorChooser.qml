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
        if(title === "Auto") {  
            setAutoColor()
            return;
        }
        parentFact.value = colorValue; 
    }

    function setAutoColor() {
        if (title !== qsTr("Auto"))
            return;
        var f = parentFact    
        if (!f)
            return;
        if(f.colorsCount <= 0) {
            f.value = "#ffffff"
            return;
        }
        var index = 0
        if(!f.parentFact.newItem) {
            index = f.parentFact.num % f.colorsCount + 1
            console.log(index)
        }
        else 
            index = f.chartsCount
        f.value = f.child(num + index).colorValue
    }
}

