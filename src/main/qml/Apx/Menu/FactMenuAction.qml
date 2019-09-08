import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import APX.Facts 1.0
import Apx.Common 1.0

CleanButton {
    id: control

    property var fact

    property int options: fact?fact.options:0
    property int dataType: fact?fact.dataType:0

    toolTip: fact?fact.descr:""
    iconName: fact?fact.icon:""
    title: (fact && (!bIconOnly))?fact.title:""
    enabled: fact?fact.enabled:true
    visible: (fact?fact.visible:true) && (bShowDisabled||enabled)

    //internal
    property bool bApply: dataType==Fact.Apply
    property bool bRemove: dataType==Fact.Remove
    property bool bStop: dataType==Fact.Stop
    property bool bPage: dataType==Fact.Page

    property bool bIconOnly: options&Fact.IconOnly
    property bool bShowDisabled: options&Fact.ShowDisabled
    property bool bClose: options&Fact.CloseOnTrigger

    color: bApply
           ? MenuStyle.cActionApply
           : bRemove
             ? MenuStyle.cActionRemove
             : bStop
               ? MenuStyle.cActionStop
               : undefined

    onTriggered: {
        if(fact) fact.trigger()
        if(bPage){
            //console.log(fact)
            if(fact && fact.bind && typeof(openFact)!='undefined')
                openFact(fact.bind)
            return
        }
        if(bClose && typeof(factMenu)!=='undefined') factMenu.back()
    }
    onMenuRequested: {
        if(typeof(fact.bind)!=='undefined')openFact(fact.bind,{"pageInfo": true, "pageInfoAction": fact})
    }

}
