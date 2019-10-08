import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Apx.Common 1.0

CleanButton {
    titleSize: 0.8
    Layout.fillHeight: true
    checkable: true
    ButtonGroup.group: buttonGroup
    property var values: []
    onActivated: signals.facts=Qt.binding(function(){return values})

    toolTip: getToolTip(values)

    function getToolTip(facts)
    {
        var s=[]
        for(var i=0;i<facts.length;++i){
            var fact=facts[i]
            s.push("<font color='"+fact.opts.color+"'>"+fact.descr+"</font>")
        }
        return s.join("<br>")
    }
}
