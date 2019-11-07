import QtQuick 2.6
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3

import Apx.Common 1.0

CleanButton {
    id: control
    property var fact

    title: fact?fact.title:""
    descr: fact?fact.descr:""
    progress: fact?fact.progress:-1

    showText: true
    showDescr: false
    showIcon: false

    toolTip: descr
    enabled: false
    hoverEnabled: true
    textAlignment: Text.AlignLeft
    font.family: font_condenced

    onTriggered: {
        if(fact) fact.trigger()
    }

    property var value: fact?fact.text:""
    property string valueText: value

    property bool active: fact?fact.active:false
    property bool warning: false
    property bool error: false
    property bool valueHighlight: false

    property bool alerts: false
    property bool alertsVehicle: true

    property real valueScale: 1

    property color normalColor: "#222"
    property color activeColor: Material.color(Material.BlueGrey)
    property color warningColor: Qt.darker(Material.color(Material.Orange),1.5)
    property color errorColor: Material.color(Material.Red)

    property color valueColor: Material.primaryTextColor
    property color valueHighlightColor: "#30ffffff"

    property real valueSize: 1.4*valueScale

    color: {
        var c
        if(error)c=errorColor
        else if(warning)c=warningColor
        else if(active)c=activeColor
        else c=normalColor //Qt.lighter(normalColor,1.0+hoverFactor*2)
        return c //hoverFactor>0?Qt.lighter(c,1.0+hoverFactor):c
    }
    Component.onCompleted: {
        background.color=Qt.binding(function(){return control.color})
        cv=apx.vehicles.current
    }

    titleColor: (active||warning||error)?Material.primaryTextColor:Qt.darker(Material.primaryTextColor,1.5)
    disabledTitleColor: titleColor

    //value
    contents: [
        Label {
            text: control.valueText
            Layout.fillWidth: !(showTitle||showIcon)
            Layout.maximumHeight: bodyHeight-Layout.topMargin
            Layout.topMargin: font.pixelSize*0.05+1
            font.family: font_narrow
            font.pixelSize: fontSize(bodyHeight*valueSize-1)
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            color: valueColor
        }
    ]

    property string message: {
        var s=[]
        s.push(title+":")
        if(descr)s.push(descr)
        s.push("("+valueText+")")
        return s.join(" ")
    }

    property bool doAlerts: alerts && (apx.datalink.valid || apx.vehicles.current.isReplay())

    property var cv

    onWarningChanged: {
        if(!warning)return
        if(alertsVehicle && cv !== apx.vehicles.current){
            cv=apx.vehicles.current
            return
        }
        if(doAlerts){
            apx.vehicles.current.warnings.warning(message)
        }
    }
    onErrorChanged: {
        if(!error)return
        if(alertsVehicle && cv !== apx.vehicles.current){
            cv=apx.vehicles.current
            return
        }
        if(doAlerts){
            apx.vehicles.current.warnings.error(message)
        }
    }

}


