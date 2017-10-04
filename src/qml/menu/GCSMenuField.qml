import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

Item {
    id: field
    width: parent?parent.width:0
    height: visible?(sep?4:itemSize):0

    visible: true
    //focus: visible

    //configurable properties
    property string title
    property bool separator: false
    property var busy: false
    property bool showBusy: !delegate

    property bool checkable: false
    property bool checked: false

    property bool enabled: true

    property Component delegate

    property string page        //page file to be loaded in body
    property string pageMenu    //page file to be loaded in stackView directly (nested menus)
    property GCSMenuModel fields //sub menu fields

    property var itemData

    signal clicked()
    signal toggled()


    //internal
    property bool sep: visible && separator && (!sephdg)
    property bool sephdg: visible && separator && title
    property bool showNext: visible && (fields || pageMenu || page) && (!delegate)


    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 1 //space between fields in list view
        border.width: 0
        color: mouseArea.containsMouse?(mouseArea.pressed?colorBgPress:colorBgHover):"#80202020" //(index&1?colorBgAlt:"transparent")
        radius: 2
        visible: field.visible

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: enabled
            enabled: field.visible && field.enabled && (!field.separator) //&& (!field.delegate)
            onClicked: {
                //console.log("click menu");
                field.focus=true;
                if(fields){
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
                }else if(closeable)close(); //click and close
                field.clicked()
            }
        }

        RowLayout {
            id: fieldBody
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.topMargin: 0
            anchors.bottomMargin: 0
            spacing: 2
            //clip: true
            Item { //field title text
                Layout.fillHeight: true
                Layout.fillWidth: !fieldDelegate.visible
                Layout.preferredWidth: titleText.contentWidth+height/4
                visible: field.visible?field.title:false
                Text { //title
                    id: titleText
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: sephdg?Text.AlignHCenter:Text.AlignLeft
                    font.pixelSize: itemSize*0.6
                    color: sephdg?colorTitleSep:"#fff"
                    font.family: sephdg?font_narrow:font_condenced
                    text: field.visible?field.title:""
                    clip: true
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
                    target: mouseArea
                    onClicked: fieldBusy.start()
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


            Component.onCompleted: if(field.visible){
                //optional field editors
                if(field.delegate){
                    field.delegate.createObject(fieldDelegate,{"anchors.fill": fieldDelegate, "modelData": field});
                }
                if(showNext){
                    fieldNextC.createObject(fieldBody);
                }
                if(field.checkable){
                    fieldSwitchC.createObject(fieldBody);
                }
            }

            Component {
                id: fieldNextC
                Image {
                    Layout.fillHeight: true
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.height: itemSize*0.8
                    source: iconNext
                }
            }

            Component {
                id: fieldSwitchC
                Switch {
                    Layout.fillHeight: true
                    //Layout.preferredWidth: height
                    anchors.verticalCenter: parent.verticalCenter
                    checked: field.checked
                    onClicked: {
                        fieldBusy.start();
                        field.clicked();
                        field.toggled();
                    }
                }
            }
        }
        FastBlur {
            anchors.fill: fieldBody
            transparentBorder: true
            source: fieldBody
            radius: fieldBody.height/2
            visible: effects && (mouseArea.containsMouse || mouseArea.pressed || field.activeFocus)
        }

    }
}
