import QtQuick 2.12
import QtQuick.Controls 2.12


ListView {
    id: listView

    clip: true

    implicitWidth: contentItem.childrenRect.width

    signal filter(var text, var v)

    property var packets: ({})

    function pid(text, color)
    {
        //console.log(text, color)

        var p=packets[text]
        if(p){
            p.count++
            _model.setProperty(p.modelIndex,"count",p.count)
        }else{
            p={}
            p.text=text
            p.count=0
            p.color=color
            p.modelIndex=_model.count
            packets[text]=p
            _model.append(p)
        }
    }

    ListModel {
        id: _model
    }


    spacing: 2

    model: _model
    delegate: DatalinkInspectorItem {
        text: model.text + " ["+model.count+"]"
        itemColor: model.color
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if(invert){
                    model.count=0
                    packets[model.text].count=0;
                }

                invert=!invert
                filter(model.text, invert)
            }
        }
    }
}
