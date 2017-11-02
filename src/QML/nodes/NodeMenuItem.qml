import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2
import GCS.FactSystem 1.0

Button {
    id: fieldButton
    anchors.fill: parent
    //anchors.left: parent.left
    //anchors.right: parent.right
    //anchors.top: parent.top
    //anchors.topMargin: 1 //space between fields in list view
    visible: field.visible

    //height: fieldItemSize*3

    enabled: field.visible && field.enabled
    Material.background: colorBgField
    flat: true
    hoverEnabled: enabled
    highlighted: enabled

    background.height: field.height-1

    onClicked: if(activeFocus){
        //console.log("click menu");
        //field.focus=true;
        if(fact)fact.trigger();
        if(fact && (fact.treeItemType!==Fact.FactItem)){
            //console.log("open: "+field);
            openFact(fact)
        }else if(fields){
            //console.log("open: "+field);
            openMenuField(field)
        }else if(field.checkable){
            //field.checked=!field.checked
            //menuItemSwitch.toggle()
        }else if(field.pageMenu){
            pushUrl(field.pageMenu)
        }else if(field.page){
            openPage({"page": Qt.resolvedUrl("../"+field.page),"title": field.title})
            //console.log("open: "+field.page);
        }else if(bEditText || bEditKey){
            //editTextDialog.open()
        }else if(closeable)close(); //click and close
        field.clicked()
        if(bAction) back();
    }

    RowLayout {
        id: fieldBody
        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        spacing: 2
        visible: !field.bAction
        //clip: true
        Item { //field title text
            id: fieldTitle
            Layout.fillHeight: true
            Layout.fillWidth: !fieldDelegate.visible
            Layout.preferredWidth: titleText.contentWidth+height/4
            visible: field.visible?field.title:false
            ColumnLayout {
                id: fieldTitleD
                anchors.fill: parent
                anchors.bottomMargin: 1
                spacing: 0
                Label { //title
                    id: titleText
                    enabled: field.enabled
                    Layout.fillWidth: true
                    Layout.fillHeight: !showDescr
                    height: font.pixelSize
                    //anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: sephdg?Text.AlignHCenter:Text.AlignLeft
                    font.pixelSize: itemSize*0.6
                    color: enabled?(bActive?colorValueText:"#fff"):"#aaa"
                    font.family: font_condenced
                    font.weight: bActive?Font.ExtraBold:Font.Normal
                    text: field.visible?field.title:""
                    clip: true
                }
                Label { //descr
                    id: descrText
                    visible: showDescr
                    Layout.fillWidth: true
                    //anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: itemSize*0.4
                    color: "#888"
                    font.family: font_condenced
                    text: field.visible?field.descr:""
                    clip: true
                }
            }
            FastBlur {
                anchors.fill: fieldTitleD
                transparentBorder: true
                source: fieldTitleD
                radius: fieldTitle.height/2
                visible: effects && fieldButton.hovered
            }
        }

        Item {
            id: fieldDelegate
            Layout.fillHeight: true
            Layout.fillWidth: true
            visible: field.delegate
        }

        BusyIndicator {
            id: fieldBusy
            running: field.busy||fieldBusyTimer.running
            visible: running && field.showBusy
            Layout.fillHeight: true
            implicitWidth: height
            Connections {
                target: fieldButton
                onClicked: if(fieldButton.activeFocus)fieldBusy.start()
            }
            Timer {
                id: fieldBusyTimer
                interval: 500; running: false; repeat: false
            }
            function start()
            {
                if(field.showBusy)fieldBusyTimer.start();
            }
        }


        Component.onCompleted: { //if(field.visible){
            //optional field editors
            if(field.delegate){
                field.delegate.createObject(fieldDelegate,{"anchors.fill": fieldDelegate, "modelData": field});
            }
            if(bConstData || bIntData || bFloatData){
                fieldConstC.createObject(fieldBody);
            }
            if(bStatus){
                fieldStatusC.createObject(fieldBody);
            }
            if(bEditText){
                fieldTextC.createObject(fieldBody);
            }
            if(bEditList || bEditListText){
                fieldListC.createObject(fieldBody);
            }
            if(bEditKey){
                fieldKeyC.createObject(fieldBody);
            }
            if(field.checkable){
                fieldSwitchC.createObject(fieldBody);
            }
            if(showNext){
                fieldNextC.createObject(fieldBody);
            }
        }


        // -------------------------------------------
        //          EDITORS
        // -------------------------------------------

        Component {
            id: fieldNextC
            Image {
                //Layout.fillHeight: true
                height: itemSize*0.8
                verticalAlignment: Image.AlignVCenter
                sourceSize.height: itemSize*0.8
                source: iconNext
            }
        }

        Component {
            id: fieldConstC
            Text {
                id: textInput
                Layout.fillHeight: true
                Layout.minimumWidth: itemSize*2
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font.pixelSize: itemSize*0.5
                color: colorValueText
                font.family: font_condenced
                text: field.fact?field.fact.text:""
            }
        }

        Component {
            id: fieldStatusC
            Text {
                visible: text
                Layout.fillHeight: true
                Layout.minimumWidth: itemSize*2
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font.pixelSize: itemSize*0.5
                color: colorStatusText
                font.family: font_condenced
                text: field.fact?field.fact.status:""
            }
        }

        Component {
            id: fieldSwitchC
            Switch {
                Layout.fillHeight: true
                //Layout.preferredWidth: height
                anchors.verticalCenter: parent.verticalCenter
                checked: field.checked
                enabled: field.enabled
                onClicked: {
                    fieldBusy.start();
                    field.toggled();
                }
            }
        }

        Component {
            id: fieldTextC
            TextInput {
                id: textInput
                Layout.fillHeight: true
                Layout.minimumWidth: itemSize*2
                //Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font.pixelSize: itemSize*0.6
                color: activeFocus?colorValueTextEdit:colorValueText
                font.family: font_condenced
                text: field.fact.text
                //clip: true
                selectByMouse: true
                onEditingFinished: {
                    field.fact.value=text;
                    parent.forceActiveFocus();
                }
                onActiveFocusChanged: {
                    if(activeFocus)selectAll();
                    //fieldButton.enabled=!activeFocus
                }
                Rectangle {
                    visible: field.enabled
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
                    target: field
                    onClicked: dialog.open()
                }
                Dialog {
                    id: dialog
                    modal: true
                    title: field.title + (field.fact.descr?" ("+field.fact.descr+")":"")
                    standardButtons: Dialog.Ok | Dialog.Cancel
                    parent: menuPage
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    implicitWidth: itemSize*10
                    onAboutToShow: {
                        editor.text=field.fact.value;
                        editor.selectAll();
                        editor.forceActiveFocus();
                    }
                    onAccepted: field.fact.value=editor.text
                    TextField {
                        id: editor
                        anchors.left: parent.left
                        anchors.right: parent.right
                        selectByMouse: true
                        placeholderText: field.fact.descr
                        onAccepted: dialog.accept()
                    }
                }
            }
        }

        Component {
            id: fieldListC
            ComboBox {
                id: editor
                Layout.fillHeight: true
                Layout.fillWidth: editable
                Layout.preferredWidth: editable?fieldBody.width:Layout.preferredWidth
                editable: field.bEditListText
                model: field.fact.enumStrings
                contentItem: editable?textInputC.createObject(editor):editor.contentItem
                background: editable?textInputBgC.createObject(editor):editor.background
                Component.onCompleted: currentIndex=find(value)
                onActivated: {
                    field.fact.value=textAt(index)
                    parent.forceActiveFocus();
                }
                property string value: field.fact.text
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
                        text: field.fact.text
                        selectByMouse: true
                        onEditingFinished: {
                            field.fact.value=text;
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
            id: fieldKeyC
            RowLayout {
                id: editor
                Layout.fillHeight: true
                Layout.fillWidth: true
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
                    text: field.fact.text
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
                        app.settings.shortcuts.blocked.value=activeFocus;
                        if(activeFocus && control.selectedText===""){
                            control.selectAll();
                        }
                    }
                    onEditingFinished: {
                        app.settings.shortcuts.blocked.value=false;
                        if(modelData) field.fact.value=text
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
