import QtQuick


Rectangle{
    id: root

    property bool activated: true
    property color activeColor: "#4cab50"
    property color defColor: "#222222"
    property string textData: "TEX"

    width: 140
    height: 50
    color: 'transparent'
    border.width: root.height * 0.06
    border.color: activated ? activeColor : defColor
    radius: 6


    Text {
        anchors.fill: parent
        color: parent.border.color
        text: qsTr(textData)
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        fontSizeMode: Text.Fit
        minimumPixelSize: 9
        elide: Text.ElideRight
        font.family: apx.font_fixed(12).family
        font.pixelSize:  parent.height * 0.62
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
