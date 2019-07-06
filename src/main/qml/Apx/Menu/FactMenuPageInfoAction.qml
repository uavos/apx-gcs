import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import APX.Facts 1.0

Flickable {
    id: infoItem
    Layout.preferredWidth: MenuStyle.itemWidth
    Layout.preferredHeight: contentHeight
    contentWidth: width
    contentHeight: layout.height
    ColumnLayout{
        id: layout
        width: parent.width
        Flow {
            spacing: 10
            Layout.fillWidth: true
            Button {
                text: qsTr("Trigger")
                onClicked: fact.trigger()
                //enabled: fact.dataType===Fact.Action
            }
        }

        TextArea {
            Layout.fillWidth: true
            selectByMouse: true
            selectByKeyboard: true
            text: getInfoText()
            textFormat: TextEdit.RichText
            wrapMode: Text.Wrap
            function getInfoText()
            {
                var s=[]
                s.push("<font color=cyan>"+fact.info().replace("\n","<br>")+"</font>")
                s.push("<h2>"+qsTr("Properties")+"</h2>")
                s.push(listProperty(fact))
                return s.join("\n")
            }
            function listProperty(item)
            {
                var s=[]
                for (var p in item)
                {
                    if(p === "objectName")continue;
                    var v=item[p]
                    if(typeof v == "function")continue;
                    if(typeof v == "object")continue;
                    if(typeof v == "undefined")continue;
                    if(v==="")continue;
                    //if(v===0)continue;
                    s.push("<b>"+p+"</b>" + ": <font color='#80ff80' face=courier>" + v +"</font>")
                }
                return s.join("<br>")
            }
        }
        Flow {
            spacing: 10
            Layout.fillWidth: true
            Button {
                text: apx.title
                onClicked: openSystemTree()
            }
        }
    }
}
