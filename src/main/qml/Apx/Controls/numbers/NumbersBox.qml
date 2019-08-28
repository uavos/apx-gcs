import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: list.implicitWidth //temWidth
    Layout.margins: 1

    readonly property real maxItems: 14
    readonly property real aspectRatio: 3.8

    readonly property real itemWidth: height/maxItems*aspectRatio
    readonly property real itemHeight: 20*ui.scale //width/aspectRatio

    property alias settingsName: numbersModel.settingsName

    NumbersModel {
        id: numbersModel
        //minimumWidth: control.implicitWidth //Math.max(control.itemHeight*3,control.implicitWidth)
        //itemWidth: control.itemWidth
        itemHeight: control.itemHeight
        //itemHeight: 20*ui.scale
        defaults: [
            {"bind":"user1","prec":"2","title":"u1"},
            {"bind":"user2","prec":"2","title":"u2"},
            {"bind":"user3","prec":"2","title":"u3"},
            {"bind":"user4","prec":"2","title":"u4"},
            {"bind":"(Math.abs(m.rc_roll.value)+Math.abs(m.rc_pitch.value))/2","title":"RC","prec":"2","warn":"value>0.2","alarm":"value>0.5"},
        ]
    }

    ListView {
        id: list
        //anchors.fill: parent
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        implicitWidth: contentWidth
        clip: true
        spacing: 0
        model: numbersModel
        snapMode: ListView.SnapToItem
        ScrollBar.vertical: ScrollBar { width: 6 }
        footer: NumbersItem {
            title: " +"
            height: control.itemHeight
            minimumWidth: height
            enabled: true
            toolTip: qsTr("Edit display values")
            onTriggered: numbersModel.edit()
        }
        onCountChanged: {
            numbersModel.minimumWidth=0
            updateWidth(0)
            numbersModel.minimumWidth=Qt.binding(function(){return control.implicitWidth})
        }
        //Component.onCompleted: updateWidth()

        function updateWidth(w)
        {
            if(typeof(w)=='undefined')w=implicitWidth
            for(var i=0;i<count;++i){
                var m=model.get(i)
                m.implicitWidthChanged.disconnect(updateWidth)
                m.implicitWidthChanged.connect(updateWidth)
                if(w>m.implicitWidth)continue
                w=m.implicitWidth
            }
            implicitWidth=Math.max(control.itemHeight*3,w)
        }
    }

}
