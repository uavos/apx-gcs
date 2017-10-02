import QtQuick 2.2
import QtQuick.Controls 1.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.2
import com.uavos.map 1.0
import "."
import "../"
import "../components"


Rectangle {
    id: menuRect
    z: 1000
    property var fields
    property alias title: titleText.text
    property alias info: infoText.text
    property int itemSize: 20*mapProvider.itemScaleFactor

    property int alignment: Qt.AlignRight


    visible: false
    property int count: fields?fields.length:0
    property color bgColor: "#C0000000"
    property color bgColorHover: "#FF404040"
    property bool fitsWidth: width<(map.width*0.4)
    property bool fitsHeight: height<map.height*0.7
    anchors.centerIn: parent
    anchors.horizontalCenterOffset: fitsWidth?(parent.width+width/2)/map.tmpScale:0
    scale: 1/map.tmpScale
    width: (itemSize)*10 //mapProvider.width/4
    height: (itemSize)*count+menuHeader.height+radius
    border.width: 0
    color: bgColor
    radius: itemSize/3
    //clip: true
    //smooth: true
    //focus: true

    //Component.onCompleted: console.log("menu: "+title);


    function show()
    {
        if(count>0)visible=true;
    }
    function hide()
    {
        visible=false;
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPressed: mouse.accepted = true //filter flickable events
        onReleased: mouse.accepted = true //filter flickable events
        onPressAndHold: mouse.accepted = true
        onWheel: mouse.accepted = true
        onPositionChanged: mouse.accepted = true
        drag.axis: Drag.XAxis
    }

    /*Image {
        id: popupImage
        anchors.horizontalCenter: menuRect.left
        anchors.verticalCenter: menuRect.verticalCenter
        width: menuRect.itemSize*2
        height: width
        source: "/icons/ionicons/android-arrow-dropleft.svg"
        smooth: true
    }
    ColorOverlay {
        id: popupImageColor
        anchors.fill: popupImage
        source: popupImage
        color: "#FFFFFF"
    }*/


    Column {
        anchors.fill: parent
        anchors.margins: menuRect.radius/2
        spacing: 0
        Item {
            id: menuHeader
            width: parent.width
            height: menuRect.itemSize*2
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 0.5
                border.width: 0
                color: "#ccf"
                smooth: true
            }
            Text {
                id: titleText
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: menuRect.itemSize*1.1
                font.family: font_narrow
                color: "white"
                visible: text!=""
                //text: modelData.caption+" "+modelData.name
            }
            FastBlur {
                anchors.fill: titleText
                //anchors.centerIn: titleText
                transparentBorder: true
                source: titleText
                radius: titleText.height/2
                visible: titleText.visible
            }
            Text {
                id: infoText
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                font.pixelSize: menuRect.itemSize*0.8
                font.family: font_narrow
                color: "gray"
                visible: text!=""
                //text: modelData.value
            }
        }
        Repeater {
            model: fields
            delegate:
            Rectangle {
                width: menuRect.width-menuRect.radius
                height: menuRect.itemSize
                border.width: 0
                color: mouseArea.containsMouse?menuRect.bgColorHover:"transparent"
                Behavior on color { ColorAnimation {duration: map.animation_duration*4; } }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true
                    //cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.log("click menu");
                        menuRect.focus=true;
                    }
                }
                Item {
                    id: menuField
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    clip: true
                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: menuRect.itemSize*0.6
                        color: "#ccf"
                        text: modelData.caption
                    }
                    MouseArea {
                        id: mouseAreaValue
                        anchors.fill: valueText
                        hoverEnabled: true
                        //propagateComposedEvents: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("click value");
                            openEditor(editorText);
                            /*valueText.visible=false;
                            editorObject=editorText.createObject(this);
                            editorObject.focus=true;
                            */
                        }
                    }
                    Text {
                        id: valueText
                        anchors.right: parent.right
                        anchors.top: parent.top
                        font.pixelSize: menuRect.itemSize//*0.9
                        font.family: font_narrow
                        color: (mouseAreaValue.containsMouse)?"#cfc":"white"
                        text: modelData.text
                    }
                }
                property var editorObject
                function closeEditor()
                {
                    if(editorObject){
                        console.log("editorObject hide");
                        editorObject.visible=false;
                        valueText.visible=true;
                        editorObject.destroy();
                        editorObject=0;
                    }
                }
                function openEditor(cmp)
                {
                    closeEditor();
                    valueText.visible=false;
                    editorObject=cmp.createObject(menuField);
                    editorObject.focus=true;
                }
                Component {
                    id: editorText
                    TextInput {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.rightMargin: 2
                        font: valueText.font
                        //font.pixelSize: menuRect.itemSize//*0.9
                        //font.family: font_narrow
                        color: "#cfc"
                        selectedTextColor: color
                        selectionColor: "#44f"
                        text: modelData.value
                        Component.onCompleted: selectAll();
                        /*onFocusChanged: {
                            if(!activeFocus)closeEditor();
                        }*/
                        //onAccepted: modelData.value=text;
                        onEditingFinished: {
                            modelData.value=text;
                            closeEditor();
                        }
                    }
                }
            }
        }
    }
}
