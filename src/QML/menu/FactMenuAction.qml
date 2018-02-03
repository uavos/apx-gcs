import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import GCS.FactSystem 1.0
import "."

Button {
    id: control
    property var factAction: modelData
    //size: height
    //Layout.fillWidth: true
    //width: height
    property bool showText: false

    property int atype: factAction?factAction.actionType:-1

    property bool bApply: atype===FactAction.ApplyAction
    property bool bRemove: atype===FactAction.RemoveAction

    property string iconName: (factAction && factAction.icon)?factAction.icon:""

    property bool bText: showText && factAction && factAction.title


    ToolTip.delay: 1000
    ToolTip.timeout: 5000
    ToolTip.visible: factAction.descr && (down || hovered)
    ToolTip.text: factAction.descr

    Material.background: bApply?Style.cActionApply:bRemove?Style.cActionRemove:undefined

    implicitHeight: visible?itemSize:0
    implicitWidth: bText?contentItem.implicitWidth+leftPadding+rightPadding:implicitHeight

    padding: 3
    leftPadding: padding+1
    rightPadding: padding+1
    topPadding: padding
    bottomPadding: padding
    spacing: 3

    background.y: 0
    background.width: width
    background.height: height-1

    enabled: factAction && factAction.enabled
    visible: factAction && factAction.visible

    text: bText?factAction.title:""
    font.pointSize: titleFontSize

    contentItem: RowLayout {
        spacing: control.spacing
        implicitHeight: itemSize
        Label {
            id: btnLabel
            visible: iconName
            Layout.fillHeight: true
            verticalAlignment: Text.AlignVCenter
            font.family: "Material Design Icons"
            font.pointSize: iconFontSize
            text: visible?materialIconChar[iconName]:""
        }
        Label {
            visible: bText
            Layout.fillHeight: true
            verticalAlignment: Text.AlignVCenter
            font: control.font
            text: control.text
        }
    }

    property bool blockPressAndHold: false

    onClicked: {
        blockPressAndHold=true
        factAction.trigger()
        blockPressAndHold=false
    }
    onPressAndHold: {
        if(blockPressAndHold)return
        openFact(fact,{"pageInfo": true, "pageInfoAction": factAction})
    }

}
