import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import APX.Facts 1.0
import APX.Protocols 1.0

RowLayout {
    id: control

    //prefs
    property int fontSize: 12


    //model data
    property alias packet: _repeater.model
    property bool uplink

    signal pid(var text, var color)

    spacing: 2

    Label {
        Layout.alignment: Qt.AlignLeft|Qt.AlignVCenter
        Layout.fillHeight: true
        text: uplink?">":"<"
        color: uplink?"#fff":"cyan"
        font.family: font_condenced
        font.pixelSize: fontSize
    }

    function get_color(type)
    {
        switch(type){
        case Protocols.PACKET: return uplink?"#aaa":"cyan"
        case Protocols.NMT: return "#faa"
        case Protocols.PID: return "#ffa"
        case Protocols.GUID: return "#a88"
        }

        return "#aaa"
    }

    Repeater {
        id: _repeater


        DatalinkInspectorItem {
            id: item
            Layout.alignment: Qt.AlignLeft|Qt.AlignVCenter
            Layout.fillHeight: true
            text: model.text

            itemColor: get_color(model.type)

            Component.onCompleted: {
                if(model.type===Protocols.PID || model.type===Protocols.NMT){
                    pid(model.text, itemColor)
                }
            }
        }
    }
    Item {
        implicitHeight: 1
        implicitWidth: 1
        Layout.fillWidth: true
    }
}
