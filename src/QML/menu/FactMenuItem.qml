import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2
import GCS.FactSystem 1.0
import "."

Item {
    id: factItem
    width: parent?parent.width:0
    height: visible?factItemSize:0

    visible: fact.visible
    //focus: visible && enabled

    property int factItemSize: (bDescr||bAction)?itemSize*1.3:itemSize

    //configurable properties
    property var parentFact
    property var fact: bEnumItemFact?factEnumElementC.createObject(factItem):modelData
    property bool bEnumItemFact: false

    onFactChanged: {
        if(!fact)fact=app
        //console.log(fact);
    }

    Component {
        id: factEnumElementC
        FactMenuElement {
            title: modelData
            onTriggered: {
                parentFact.setValue(modelData)
                back();
            }
        }
    }

    //internal
    property bool bNext: fact.size || fact.treeItemType===Fact.GroupItem || bEnumChilds
    property bool bDescr: fact.descr && app.settings.showdescr.value
    property bool bEnumChilds: fact.dataType===Fact.EnumData && fact.enumStrings.length>maxEnumListSize

    //editor types
    property bool bAction:       fact.dataType===Fact.ActionData && (fact.value?fact.value:false)
    property bool bEditText:     fact.dataType===Fact.TextData && fact.enumStrings.length === 0
    property bool bEditList:     fact.dataType===Fact.EnumData && (!bEnumChilds)
    property bool bEditListText: fact.dataType===Fact.TextData && fact.enumStrings.length > 0
    property bool bEditBool:     fact.dataType===Fact.BoolData
    property bool bEditKey:      fact.dataType===Fact.KeySequenceData
    property bool bConst:        fact.dataType===Fact.ConstData
    property bool bEditInt:      fact.dataType===Fact.IntData && fact.enumStrings.length === 0
    property bool bEditFloat:    fact.dataType===Fact.FloatData

    Button {
        id: factItemButton
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: bAction?undefined:parent.right
        anchors.topMargin: 1 //space between items in list view

        enabled: fact.enabled
        Material.background: bAction?(fact.value===Fact.RemoveAction?colorActionRemove:undefined):colorBgField
        flat: !bAction
        hoverEnabled: enabled
        highlighted: enabled

        background.height: bAction?(factItem.height-background.y*2):(factItem.height-1)
        background.y: bAction?background.y:0

        text: bAction?fact.title:""

        //focus: false

        onClicked: if(activeFocus){
            //console.log("click menu");
            //factItem.focus=true;
            fact.trigger();
            if(bNext) openFact(fact)
            else if(bEnumChilds) openFact(fact)
            else if(bAction && fact.value!==Fact.RemoveAction) back();
        }

        Item { // title & descr text
            id: factItemTitle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.topMargin: 0
            anchors.bottomMargin: 1
            visible: fact.title && !bAction
            ColumnLayout {
                id: factItemTitleD
                anchors.fill: parent
                anchors.bottomMargin: 1
                spacing: 0
                Label { //title
                    id: titleText
                    enabled: fact.enabled
                    Layout.fillWidth: true
                    Layout.fillHeight: !bDescr
                    height: itemSize*0.6
                    verticalAlignment: Text.AlignVCenter
                    //horizontalAlignment: Text.AlignLeft
                    font.pixelSize: itemSize*0.6
                    color: enabled?(fact.active?colorValueText:"#fff"):"#aaa"
                    font.family: font_condenced
                    text: fact.title
                }
                Label { //descr
                    id: descrText
                    visible: bDescr
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                    //horizontalAlignment: Text.AlignLeft
                    font.pixelSize: itemSize*0.4
                    color: "#aaa"
                    font.family: font_condenced
                    text: fact.descr
                }
            }
            FastBlur {
                anchors.fill: factItemTitleD
                transparentBorder: true
                source: factItemTitleD
                radius: factItemTitle.height/2
                visible: effects && factItemButton.hovered
            }
        } //title block

        //NEXT icon
        Image {
            id: factItemNext
            visible: bNext
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: height
            height: itemSize*0.8
            sourceSize.height: height
            source: iconNext
        }

        RowLayout {
            id: factItemBody
            spacing: 2
            visible: !bAction
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: factItemNext.visible?factItemNext.left:parent.right
            anchors.rightMargin: 4
            //height: parent.height
            //clip: true


            ProgressBar {
                property int v: fact.progress
                visible: v>0
                value: v/100
                indeterminate: v<0
            }

            BusyIndicator {
                id: factItemBusy
                running: fact.busy||factBusyTimer.running
                visible: running
                Layout.fillHeight: true
                implicitWidth: height
                Connections {
                    target: fact
                    onBusyChanged: if(fact.busy)factBusyTimer.start()
                }
                Timer {
                    id: factBusyTimer
                    interval: 500; running: false; repeat: false
                    onTriggered: if(fact.busy)factBusyTimer.start()
                }
            }

            property Item factEditor

            //Fact status
            Text {
                visible: text
                Layout.fillHeight: true
                Layout.fillWidth: factItemBody.factEditor?false:true
                Layout.minimumWidth: contentWidth
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font.pixelSize: itemSize*0.5
                color: colorStatusText
                font.family: font_condenced
                text: fact.status
            }

            //EDITOR
            property Component editorComponent:
                bEditText?factItemTextC:
                bEditBool?factItemSwitchC:
                (bEditList || bEditListText)?factItemListC:
                bEditKey?factItemKeyC:
                factItemConstC;

            onEditorComponentChanged: {
                if(factEditor) {
                    factEditor.destroy();
                    factEditor=undefined
                }
                if(factItem.fact.dataType!=Fact.NoData){
                    factEditor=editorComponent.createObject(factItemBody);
                }
            }

            // -------------------------------------------
            //          EDITORS
            // -------------------------------------------

            Component {
                id: factItemConstC
                Text {
                    Layout.fillHeight: true
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: itemSize*0.5
                    color: colorValueText
                    font.family: font_condenced
                    text: fact.text
                }
            }

            Component {
                id: factItemSwitchC
                Switch {
                    Layout.fillHeight: true
                    //Layout.preferredWidth: height
                    anchors.verticalCenter: parent.verticalCenter
                    checked: fact.value
                    enabled: fact.enabled
                    onClicked: fact.setValue(fact.value?false:true)
                }
            }

            Component {
                id: factItemTextC
                TextInput {
                    id: textInput
                    Layout.fillHeight: true
                    Layout.minimumWidth: itemSize*2
                    //Layout.fillWidth: true
                    //anchors.right: parent.right
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: itemSize*0.6
                    color: activeFocus?colorValueTextEdit:colorValueText
                    font.family: font_condenced
                    text: fact.text
                    //clip: true
                    selectByMouse: true
                    onEditingFinished: {
                        fact.setValue(text);
                        parent.forceActiveFocus();
                    }
                    onActiveFocusChanged: {
                        if(activeFocus)selectAll();
                        //factItemButton.enabled=!activeFocus
                    }
                    Rectangle {
                        visible: fact.enabled
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: -itemSize*0.05
                        anchors.rightMargin: anchors.leftMargin
                        anchors.verticalCenter: parent.verticalCenter
                        height: itemSize*0.8
                        radius: 3
                        color: "transparent"
                        border.width: 1
                        border.color: parent.color
                        opacity: 0.3
                    }
                    Connections {
                        target: factItemButton
                        onClicked: dialog.open()
                    }
                    Dialog {
                        id: dialog
                        modal: true
                        title: fact.title + (fact.descr?" ("+fact.descr+")":"")
                        standardButtons: Dialog.Ok | Dialog.Cancel
                        parent: menuPage
                        x: (parent.width - width) / 2
                        y: (parent.height - height) / 2
                        implicitWidth: itemSize*10
                        onAboutToShow: {
                            editor.text=fact.text;
                            editor.selectAll();
                            editor.forceActiveFocus();
                        }
                        onAccepted: fact.setValue(editor.text)
                        TextField {
                            id: editor
                            anchors.left: parent.left
                            anchors.right: parent.right
                            selectByMouse: true
                            placeholderText: fact.descr
                            onAccepted: dialog.accept()
                        }
                    }
                }
            }

            Component {
                id: factItemListC
                ComboBox {
                    id: editor
                    //height: parent.height
                    //implicitWidth: editable?factItemBody.width:implicitWidth
                    Layout.fillHeight: true
                    Layout.fillWidth: editable
                    Layout.preferredWidth: editable?(factItemButton.width*0.7):Layout.preferredWidth
                    //popup.width: factItemButton.width*0.7
                    editable: bEditListText
                    model: fact.enumStrings
                    contentItem: editable?textInputC.createObject(editor):editor.contentItem
                    background: editable?textInputBgC.createObject(editor):editor.background
                    Component.onCompleted: currentIndex=find(value)
                    onActivated: {
                        fact.setValue(textAt(index))
                        parent.forceActiveFocus();
                    }
                    property string value: fact.text
                    onValueChanged: currentIndex=find(value)
                    Connections {
                        target: listView
                        onMovementStarted: editor.popup.close()
                    }
                    Component {
                        id: textInputC
                        TextInput {
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            font.pixelSize: itemSize*0.6
                            color: activeFocus?colorValueTextEdit:colorValueText
                            font.family: font_condenced
                            text: fact.text
                            selectByMouse: true
                            onEditingFinished: {
                                fact.setValue(text);
                                editor.parent.forceActiveFocus();
                            }
                            onActiveFocusChanged: if(activeFocus)selectAll();
                        }
                    }
                    Component {
                        id: textInputBgC
                        Item {}
                    }
                }
            }

            Component {
                id: factItemKeyC
                RowLayout {
                    id: editor
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Shortcut {
                        id: scText
                        enabled: false
                        sequence: (s.length>1 && s.endsWith("+"))?s.substr(0,s.length-1):s
                        property alias s: control.text
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        Layout.fillWidth: true
                        anchors.leftMargin: font.pixelSize
                        verticalAlignment: Qt.AlignVCenter
                        font.family: font_condenced
                        font.pixelSize: itemSize*0.6
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
                        height: itemSize
                        font.family: font_condenced
                        font.pixelSize: itemSize*0.6
                        placeholderText: qsTr("Key Sequence")
                        text: fact.text
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
                            app.settings.shortcuts.blocked.setValue(activeFocus);
                            if(activeFocus && control.selectedText===""){
                                control.selectAll();
                            }
                        }
                        onEditingFinished: {
                            app.settings.shortcuts.blocked.setValue(false);
                            if(modelData) fact.setValue(text)
                        }
                        //Keys.onEscapePressed: editor.parent.forceActiveFocus();
                        Keys.onPressed: {
                            //console.log("key: "+event.key+" text: "+event.text)
                            event.accepted=true
                            control.remove(control.selectionStart,control.selectionEnd);
                            var s=app.settings.shortcuts.keyToPortableString(event.key,event.modifiers);
                            var i=control.cursorPosition;
                            if(control.text.endsWith('+'))i=control.text.length;
                            control.insert(i,s);
                            if(!control.text.endsWith('+'))control.selectAll();
                        }
                    }
                }
            }
        }
    }
}


