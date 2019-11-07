import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Button {
    id: control

    property string iconName
    property var color

    property color titleColor: Material.primaryTextColor
    property color disabledTitleColor: Material.hintTextColor
    property color iconColor: Material.iconColor

    property int textAlignment: Text.AlignHCenter
    property int defaultHeight: 32

    property string title: text
    property string descr
    property string toolTip: descr?descr:title

    property int progress: -1

    property bool showIcon: iconName
    property bool showTitle: title
    property bool showText: false
    property bool showDescr: true

    property int minimumWidth: 0

    property real ui_scale: ui.scale

    signal triggered()
    signal activated()

    focus: false

    //internal
    property bool showContents: (iconName&&showIcon)?showText && title:true

    padding: 2*ui_scale //iconName?3:3
    leftPadding: padding+1
    rightPadding: padding+1
    topPadding: padding
    bottomPadding: padding
    spacing: 3*ui_scale

    topInset: 0
    bottomInset: 1

    background.y: 0
    background.width: width
    background.height: height-1

    highlighted: activeFocus

    Material.background: color?color:undefined //"#80303030"

    Material.theme: Material.Dark
    Material.accent: Material.color(Material.BlueGrey)
    Material.primary: Material.color(Material.LightGreen)


    //font.family: font_app
    font.pixelSize: 14*ui_scale //Qt.application.font.pixelSize*ui_scale

    //implicitHeight: visible?contentItem.implicitHeight+topPadding+bottomPadding:0
    implicitHeight: defaultHeight*ui_scale
    implicitWidth: defaultWidth

    property int defaultWidth: showContents?Math.max(implicitHeight,Math.max(minimumWidth,contentItem.implicitWidth+leftPadding+rightPadding)):implicitHeight


    ToolTip.delay: 1000
    ToolTip.timeout: 5000
    ToolTip.visible: ToolTip.text && (down || hovered)
    ToolTip.text: toolTip

    property alias contents: bodyLayout.children

    property real bodyHeight: height-padding*2

    property real iconSize: 0.9
    property real titleSize: showDescr?0.6:0.9
    property real descrSize: 0.35

    function fontSize(v){return Math.max(7,v)}

    contentItem: Item {
        implicitWidth: rowItem.implicitWidth
        height: bodyHeight
        RowLayout {
            id: rowItem
            spacing: 0
            height: bodyHeight
            width: parent.width
            //clip: true
            //icon
            Loader {
                id: iconItem
                active: showIcon && control.iconName
                //asynchronous: true
                Layout.fillHeight: true
                //Layout.fillWidth: !showContents
                Layout.alignment: Qt.AlignCenter
                sourceComponent: Component {
                    MaterialIcon {
                        verticalAlignment: Text.AlignVCenter
                        //horizontalAlignment: showContents?Text.AlignLeft:Text.AlignHCenter
                        //font.family: "Material Design Icons"
                        size: fontSize(bodyHeight*iconSize)
                        name: iconName
                        color: control.enabled?control.iconColor:Material.iconDisabledColor
                    }
                }
                //BoundingRect{}
            }

            //Title
            Loader {
                id: titleItem
                active: control.showTitle
                //asynchronous: true
                Layout.maximumHeight: bodyHeight+control.padding
                Layout.fillWidth: active
                Layout.leftMargin: (active&&iconItem.active)?control.spacing*2:0
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: -control.padding
                //Layout.bottomMargin: -control.padding
                sourceComponent: Component {
                    Item {
                        id: titleLayout
                        visible: showContents
                        implicitHeight: bodyHeight+control.padding
                        implicitWidth: Math.max(titleText.implicitWidth+10,descrText.visible?descrText.implicitWidth+10:0)
                        //implicitHeight: titleText.implicitHeight+(descrText.visible?descrText.implicitHeight:0)
                        //clip: true
                        Label {
                            id: titleText
                            anchors.fill: parent
                            verticalAlignment: descrText.visible?Text.AlignTop:Text.AlignVCenter
                            horizontalAlignment: textAlignment
                            font.family: control.font.family
                            font.pixelSize: fontSize(bodyHeight*titleSize)
                            text: control.title
                            color: control.enabled?titleColor:disabledTitleColor
                        }
                        Label {
                            id: descrText
                            anchors.fill: parent
                            visible: showDescr && text
                            verticalAlignment: Text.AlignBottom
                            horizontalAlignment: textAlignment
                            font.family: control.font.family
                            font.pixelSize: fontSize(bodyHeight*descrSize)
                            text: control.descr
                            color: control.enabled?Material.secondaryTextColor:Material.hintTextColor
                        }
                    }
                }
                //BoundingRect{}
            }
            //stretch space
            Item {
                visible: control.showIcon||control.showTitle
                Layout.fillHeight: showContents
                Layout.fillWidth: showContents && visible
            }
            //tools
            RowLayout {
                id: bodyLayout
                //Layout.alignment: Qt.AlignRight
                Layout.fillHeight: true
                //Layout.alignment: Qt.AlignTop
                Layout.maximumHeight: bodyHeight
                spacing: 0
                visible: showContents
            }
        }
        ProgressBar {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 1
            anchors.leftMargin: anchors.margins+(iconName?bodyHeight:0)
            background.height: height-anchors.margins*2
            contentItem.implicitHeight: control.contentItem.height-anchors.margins*2
            opacity: 0.33
            to: 100
            property int v: control.progress
            value: v
            visible: v>=0
            indeterminate: v==0
            Material.accent: Material.color(Material.Green)
        }
    }


    onClicked: {
        triggered()
    }

    onCheckedChanged: if(checked)activated()

    onActiveFocusChanged: {
        focusTimer.start()
    }
    Timer {
        id: focusTimer
        interval: 100
        onTriggered: {
            if(control.pressed)start()
            else control.focus=false
        }
    }
}
