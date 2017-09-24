import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3

Rectangle {
    id: menuField
    width: listView.width
    height: sep?4:itemSize
    border.width: 0
    color: mouseArea.containsMouse?(mouseArea.pressed?colorBgPress:root.colorBgHover):"#80202020" //(index&1?colorBgAlt:"transparent")
    radius: 2

    property bool sep: modelData.separator && (!sephdg)
    property bool sephdg: modelData.separator && modelData.title

    RowLayout {
        id: menuFieldBody
        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        spacing: 0
        clip: true
        Text { //title
            Layout.fillHeight: true
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: sephdg?Text.AlignHCenter:Text.AlignLeft
            font.pixelSize: root.itemSize*0.6
            color: sephdg?colorTitleSep:"#fff"
            font.family: sephdg?font_narrow:font_condenced
            text: modelData.title
            clip: true
        }
        BusyIndicator {
            id: menuFieldBusy
            running: (modelData.busy!=="undefined")?modelData.busy:false
            visible: running
            Layout.fillHeight: true
            implicitWidth: height
            function start()
            {
                menuFieldBusy.running=true;
                menuFieldBusyTimer.start();
            }
            Timer {
                id: menuFieldBusyTimer
                interval: 500; running: false; repeat: false
                onTriggered: menuFieldBusy.running=(modelData.busy!=="undefined")?Qt.binding(function() { return modelData.busy }):false
            }
        }
        //optional field editors
        Loader { //submenu
            active: modelData.contents.length || modelData.subMenu || modelData.page
            visible: active
            Layout.fillHeight: true
            Layout.margins: 4
            sourceComponent: Image {
                anchors.verticalCenter: parent.verticalCenter
                sourceSize.height: root.btnSizeNext
                source: "navigation_next_item.png"
            }
        }
        Loader { //switch
            active: modelData.checkable
            visible: active
            Layout.fillHeight: true
            sourceComponent: Component {
                Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: modelData.checked
                    onClicked: {
                        menuFieldBusy.start();
                        modelData.clicked()
                    }
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: !modelData.separator
        //propagateComposedEvents: true
        //cursorShape: Qt.PointingHandCursor
        onClicked: {
            //console.log("click menu");
            menuFieldBusy.start();
            root.focus=true;
            modelData.clicked()
            if(modelData.contents.length>0){
                openPage(modelData)
            }else if(modelData.checkable){
                //modelData.checked=!modelData.checked
                //menuItemSwitch.toggle()
            }else if(modelData.subMenu){
                stackView.push(Qt.resolvedUrl("../"+modelData.subMenu),{"parentStack": stackView})
            }else if(modelData.page){
                stackView.push(pageDelegate.createObject(),{"page": Qt.resolvedUrl("../"+modelData.page),"title": modelData.title})
            }else if(closeable)root.close();
        }
    }
}
