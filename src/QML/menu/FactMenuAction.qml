import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import GCS.FactSystem 1.0
import "."

Button {
    property var fact
    //size: height
    //Layout.fillWidth: true
    //width: height

    property int atype: fact?fact.value:-1

    property bool bApply: atype===Fact.ApplyAction
    property bool bRemove: atype===Fact.RemoveAction

    property string iconName: (fact && fact.iconSource)?fact.iconSource:bApply?"check":bRemove?"delete":""

    Material.background: bApply?Style.cActionApply:bRemove?Style.cActionRemove:undefined
    //anchors.topMargin: 0
    //anchors.bottomMargin: 0

    enabled: fact && fact.enabled

    text: fact?fact.title:""
    leftPadding: rightPadding+(btnLabel.visible?height*0.6:0)
    Label {
        id: btnLabel
        visible: iconName
        anchors.fill: parent
        anchors.leftMargin: parent.rightPadding
        verticalAlignment: Text.AlignVCenter
        font.family: "Material Design Icons"
        font.pointSize: parent.implicitHeight*0.6
        text: visible?materialIconChar[iconName]:""
    }
    onClicked: {
        //console.log(materialIconChar[iconName])
        //if(closeable)close();
        //else back()
        fact.trigger()
    }
}
