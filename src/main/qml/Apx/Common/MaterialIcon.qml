import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Text {
    id: control

    property string name
    property int size: 32

    font.family: "Material Design Icons"
    font.pixelSize: control.size
    text: (name&&visible)?application.materialIconChar(name):""
    color: "#fff"
}
