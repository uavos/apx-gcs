import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import APX.Facts 1.0
import Apx.Common 1.0

CleanButton {
    id: control

    property var factAction

    property int flags: (factAction && factAction.flags)?factAction.flags:0

    toolTip: factAction?factAction.descr:""
    iconName: factAction?factAction.icon:""
    title: ((!bHideTitle)&&factAction)?factAction.title:""
    enabled: factAction?factAction.enabled:true
    visible: factAction?factAction.visible:true

    //internal
    property bool bApply: flags&FactAction.ActionApply
    property bool bRemove: flags&FactAction.ActionRemove
    property bool bStop: flags&FactAction.ActionStop
    property bool bClose: flags&FactAction.ActionCloseOnTrigger
    property bool bPage: flags&FactAction.ActionPage
    property bool bHideTitle: flags&FactAction.ActionHideTitle

    color: bApply?MenuStyle.cActionApply:
                   bRemove?MenuStyle.cActionRemove:
                            bStop?MenuStyle.cActionStop:
                                   undefined

    onTriggered: {
        if(bPage){
            //console.log(factAction.fact)
            if(factAction){
                factAction.trigger()
                if(typeof(openFact)!='undefined') openFact(factAction.fact)
            }
            return
        }
        if(factAction) factAction.trigger()
        if(bClose && typeof(factMenu)!=='undefined') factMenu.back()
    }
    onMenuRequested: {
        if(typeof(fact)!=='undefined')openFact(fact,{"pageInfo": true, "pageInfoAction": factAction})
    }

}
