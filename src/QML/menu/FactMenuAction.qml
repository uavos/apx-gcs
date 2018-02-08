import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import GCS.FactSystem 1.0

import "../components"
import "."

CleanButton {
    id: control
    property var factAction: modelData

    property int flags: factAction?factAction.flags:-1

    toolTip: factAction.descr
    iconName: factAction?factAction.icon:""
    text: factAction?factAction.title:""
    enabled: factAction && factAction.enabled
    visible: factAction && factAction.visible

    font.pointSize: titleFontSize

    //internal
    property bool bApply: flags&FactAction.ActionApply
    property bool bRemove: flags&FactAction.ActionRemove
    property bool bClose: flags&FactAction.ActionCloseOnTrigger
    property bool bPage: flags&FactAction.ActionPage

    color: bApply?Style.cActionApply:bRemove?Style.cActionRemove:undefined



    onTriggered: {
        if(bPage){
            //console.log(factAction.fact)
            openFact(factAction.fact)
            return
        }
        factAction.trigger()
        if(bClose)factMenu.back()
    }
    onMenuRequested: {
        if(fact)openFact(fact,{"pageInfo": true, "pageInfoAction": factAction})
    }

}
