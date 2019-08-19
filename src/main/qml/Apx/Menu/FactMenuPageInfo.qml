import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Flickable {
    id: infoItem

    property var factAction

    implicitWidth: MenuStyle.itemWidth
    implicitHeight: contentHeight
    contentWidth: width
    contentHeight: layout.height
    ColumnLayout{
        id: layout
        width: parent.width
        Flow {
            spacing: 10
            Layout.fillWidth: true
            Button {
                text: qsTr("Revert")
                onClicked: fact.restore()
                enabled: fact.modified
                visible: !factAction
            }
            Button {
                text: qsTr("Trigger")
                onClicked: {
                    if(factAction)factAction.trigger()
                    else fact.trigger()
                }
                enabled: factAction?factAction.enabled:fact.enabled
            }
        }

        TextArea {
            Layout.fillWidth: true
            //Layout.preferredHeight: width
            selectByMouse: true
            selectByKeyboard: true
            text: getInfoText()
            textFormat: TextEdit.RichText
            wrapMode: Text.Wrap
            function getInfoText()
            {
                var s=[]
                if(factAction){
                    s.push("<h2>"+qsTr("Action info")+"</h2>")
                    s.push("<font color=cyan>"+factAction.title+"</font><br>")
                    s.push(factAction.descr)
                }
                s.push("<h2>"+qsTr("Fact info")+"</h2>")
                s.push("<font color=cyan>"+fact.info().replace(/\n/g,"<br>")+"</font>")
                s.push("<h2>"+qsTr("Properties")+"</h2>")
                s.push(listProperty(fact))
                s.push(listActions(fact))
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
            function listActions(item)
            {
                var s=[]
                for (var p in item.actions)
                {
                    var v=item.actions[p]
                    if(typeof v !== "object")continue;
                    if(s.length==0)s.push("<h2>"+qsTr("Actions")+"</h2>")
                    s.push("<b>"+p+"</b>" + ": <font color='#80ff80' face=courier>" + v.descr +"</font><br>")
                }
                return s.join("\n")
            }
        }
        /*Flow {
            spacing: 10
            Layout.fillWidth: true
            Button {
                text: apx.title
                onClicked: openSystemTree()
            }
        }*/
    }
}
