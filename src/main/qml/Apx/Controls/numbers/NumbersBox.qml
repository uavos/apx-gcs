﻿import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: layout.implicitWidth
    implicitHeight: 200

    property int margins: 3

    property real itemSize: 20*ui.scale
    property bool showEditButton: true

    property alias settingsName: numbersModel.settingsName
    property alias defaults: numbersModel.defaults
    property alias model: numbersModel

    NumbersModel {
        id: numbersModel
        itemHeight: control.itemSize
        fixedWidth: true
        defaults: [
            {"bind":"user1","prec":"2","title":"u1"},
            {"bind":"user2","prec":"2","title":"u2"},
            {"bind":"user3","prec":"2","title":"u3"},
            {"bind":"user4","prec":"2","title":"u4"},
            {"bind":"(Math.abs(m.rc_roll.value)+Math.abs(m.rc_pitch.value))/2","title":"RC","prec":"2","warn":"value>0.2","alarm":"value>0.5"},
        ]
    }

    ColumnLayout{
        id: layout
        height: control.height
        ListView {
            id: list
            Layout.margins: control.margins
            Layout.fillHeight: true
            implicitWidth: numbersModel.minimumWidth
            clip: true
            spacing: 0
            model: numbersModel
            snapMode: ListView.SnapToItem
            ScrollBar.vertical: ScrollBar { width: 6 }
            footer: Loader{
                active: showEditButton
                sourceComponent: NumbersItem {
                    title: " +"
                    height: control.itemSize
                    minimumWidth: height
                    enabled: true
                    toolTip: qsTr("Edit display values")
                    onTriggered: numbersModel.edit()
                }
            }
            /*onCountChanged: {
                numbersModel.minimumWidth=0
                updateWidth(0)
                numbersModel.minimumWidth=Qt.binding(function(){return layout.implicitWidth-control.margins*2})
            }

            function updateWidth(w)
            {
                if(typeof(w)=='undefined')w=implicitWidth
                for(var i=0;i<count;++i){
                    var m=model.get(i)
                   // m.implicitWidthChanged.disconnect(updateWidth)
                   // m.implicitWidthChanged.connect(updateWidth)
                    if(w>m.implicitWidth)continue
                    w=m.implicitWidth
                }
                //implicitWidth=Math.max(control.itemSize*3,w)
            }*/
        }
    }
}
