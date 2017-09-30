import QtQuick 2.7
import QtQml 2.2
import QtQml.Models 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "./components"

GCSMenu {
    id: menuShortcuts
    title: qsTr("Shortcuts")

    fields: ObjectModel {
        id: objModel
        GCSMenuField { title: qsTr("User shortcuts"); separator: true; }
        GCSMenuField {
            title: qsTr("Add new");
            Component.onCompleted: addEditor(this,app.shortcuts.newItem)
        }
        GCSMenuField { title: qsTr("Enable all"); onClicked: app.shortcuts.enableAllUser(true);}
        GCSMenuField { title: qsTr("Disable all"); onClicked: app.shortcuts.enableAllUser(false);}
        GCSMenuList {
            objModel: objModel
            model: app.shortcuts.userShortcuts
            delegate: GCSMenuField {
                showBusy: false
                delegate: scElement
                checkable: true
                checked: item.enabled
                onToggled: item.enabled=!item.enabled
                itemData: item
                Component.onCompleted: addEditor(this,item)
            }
        }

        GCSMenuField { title: qsTr("System shortcuts"); separator: true; }
        GCSMenuField { title: qsTr("Enable all"); onClicked: app.shortcuts.enableAllSystem(true);}
        GCSMenuField { title: qsTr("Disable all"); onClicked: app.shortcuts.enableAllSystem(false);}
        GCSMenuList {
            objModel: objModel
            model: app.shortcuts.systemShortcuts
            delegate: GCSMenuField {
                showBusy: false
                delegate: scElement
                checkable: true
                checked: item.enabled
                onToggled: item.enabled=!item.enabled
                itemData: item
                Component.onCompleted: addEditor(this,item)
            }
        }
    }

    function addEditor(parent,scData)
    {
        var obj=scEditor.createObject(parent);
        parent.fields=obj;
        for(var i=0;i<obj.count;i++){
            var iobj=obj.get(i);
            iobj.itemData=scData
            //console.log(iobj)
        }
    }

    Component {
        id: scElement
        RowLayout {
            property variant modelData
            Btn {
                active: false
                //onClicked: app.shortcuts.systemShortcuts.removeItem(modelData.itemData);
                Layout.alignment: Qt.AlignLeft
                Layout.fillHeight: true
                text: scText.nativeText
                effects: root.effects
                font.family: font_condenced
                font.pixelSize: root.itemSize*0.6
                color: modelData.itemData.enabled?"#478fff":"transparent"
                Shortcut {
                    id: scText
                    enabled: false
                    sequence: modelData.itemData.key
                }
            }
            Btn {
                active: false
                Layout.alignment: Qt.AlignRight
                Layout.fillHeight: true
                text: modelData.itemData.cmd.slice(-20)
                effects: root.effects
                font.family: font_condenced
                font.pixelSize: root.itemSize*0.6
                color: "gray"
            }
        }
    }

    Component {
        id: scElementKey
        RowLayout {
            //anchors.fill: parent
            property variant modelData
            Shortcut {
                id: scText
                enabled: false
                sequence: control.text
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                Layout.fillWidth: true
                anchors.leftMargin: font.pixelSize
                verticalAlignment: Qt.AlignVCenter
                font.family: font_condenced
                font.pixelSize: root.itemSize*0.6
                color: colorBgPress
                text: scText.nativeText
            }
            TextField {
                id: control
                focus: true
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                Layout.fillHeight: true
                //anchors.verticalCenter: parent.verticalCenter
                topPadding: 0
                bottomPadding: 0
                height: root.itemSize
                font.family: font_condenced
                font.pixelSize: root.itemSize*0.6
                placeholderText: qsTr("Key Sequence")
                text: modelData.itemData.key
                hoverEnabled: true
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignRight
                selectByMouse: true
                persistentSelection: true
                background: Rectangle {
                    anchors.fill: parent
                    color: control.activeFocus ? colorBgHover : "transparent"
                    border.width: 0
                }
                onActiveFocusChanged: {
                    app.shortcuts.blocked=activeFocus;
                    if(activeFocus && control.selectedText===""){
                        control.selectAll();
                    }
                }
                onEditingFinished: {
                    app.shortcuts.blocked=false;
                    if(modelData) modelData.itemData.key=text
                }
                Keys.onPressed: {
                    //console.log("key: "+event.key+" text: "+event.text)
                    event.accepted=true
                    //control.text=app.shortcuts.keyToPortableString(event.key,event.modifiers)
                    //control.selectAll();
                    control.remove(control.selectionStart,control.selectionEnd);
                    var s=app.shortcuts.keyToPortableString(event.key,event.modifiers);
                    var i=control.cursorPosition;
                    if(control.text.endsWith('+'))i=control.text.length;
                    control.insert(i,s);
                    if(!control.text.endsWith('+'))control.selectAll();
                }
            }
        }
    }

    Component {
        id: scElementCmd
        RowLayout {
            property variant modelData
            TextField {
                id: control
                focus: true
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                Layout.fillHeight: true
                topPadding: 0
                bottomPadding: 0
                height: root.itemSize
                font.family: font_condenced
                font.pixelSize: root.itemSize*0.6
                placeholderText: qsTr("Java Script")
                text: modelData.itemData.cmd
                hoverEnabled: true
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignRight
                selectByMouse: true
                persistentSelection: true
                background: Rectangle {
                    anchors.fill: parent
                    color: control.activeFocus ? colorBgHover : "transparent"
                    border.width: 0
                }
                onActiveFocusChanged: {
                    if(activeFocus && control.selectedText===""){
                        control.selectAll();
                    }
                }
                onEditingFinished: {
                    if(modelData) modelData.itemData.cmd=text
                }
            }
        }
    }

    Component {
        id: scEditor
        ObjectModel {
            //title: qsTr("Edit Shortcut");
            GCSMenuField { title: qsTr("Enable"); checkable: true; checked: itemData.enabled; onClicked: itemData.enabled=!itemData.enabled; }
            GCSMenuField {
                title: qsTr("Delete");
                visible: itemData !== app.shortcuts.newItem
                onClicked: {
                    app.shortcuts.systemShortcuts.removeItem(itemData);
                    menuShortcuts.back();
                }
            }
            GCSMenuField {
                title: qsTr("Add");
                visible: itemData === app.shortcuts.newItem
                enabled: itemData.valid
                onClicked: {
                    app.shortcuts.addNew();
                    menuShortcuts.back();
                }
            }
            GCSMenuField { title: qsTr("Key"); delegate: scElementKey; }
            GCSMenuField { title: qsTr("Script"); delegate: scElementCmd; }
        }

    }
}
