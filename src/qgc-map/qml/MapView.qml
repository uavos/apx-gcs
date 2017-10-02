import QtQuick 2.2
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2
import QtQuick.Layouts 1.2
import com.uavos.map 1.0
import "."
import "./components"
import "./vehicle"
import "./mission"
import "./menu"


QmlMap {
    id: mapProvider
    property int btnSize: 2*Math.max(Math.min(width,height)/50,Math.max(20,15*mapScaleFactor))

    QmlMissionModel {
        id: mission
    }

    FlickableMap {
        id: map
        anchors.top: parent.top
        anchors.bottom: statusBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        Mission {}
        Vehicles {}
    }

    MapStatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 18*mapProvider.mapScaleFactor
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            border.width: 0
            color: "#808080FF"
        }
    }


    ColumnLayout {
        id: buttonsLeft
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: statusBar.top
        anchors.margins: 4
        //spacing: 5//btnSize/2
        width: btnSize
        clip: true

        MapButton {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: btnSize
            Layout.preferredHeight: btnSize
            icon: "android-menu"
            //onClicked: console.log(btnSize)
        }
        Item { Layout.fillHeight: true }

        MapButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: btnSize//*0.7
            Layout.preferredHeight: btnSize//*0.7
            //icons: "navigation"
            //icon: "star"
            icon: "android-plane"
            //overIcon: "android-add-circle"
            color: "#ccf"//"#ccf"
        }
        Item { Layout.fillHeight: true; Layout.maximumHeight: btnSize; }
        MapButton {
            Layout.preferredWidth: btnSize
            Layout.preferredHeight: btnSize
            icons: "navigation"
            icon: "placeholder-19"
            color: "#f88"
            onClicked: map.deepDownload();
        }
        Item { Layout.fillHeight: true; Layout.maximumHeight: btnSize; }
        MapButton {
            Layout.preferredWidth: btnSize
            Layout.preferredHeight: btnSize
            icon: "ios-plus"
            onClicked: map.zoomIn(mapProvider.center);
        }
        MapButton {
            Layout.preferredWidth: btnSize
            Layout.preferredHeight: btnSize
            icon: "ios-minus"
            onClicked: map.zoomOut(mapProvider.center);
        }
        Item { Layout.fillHeight: true; Layout.maximumHeight: btnSize; }
        MapButton {
            Layout.alignment: Qt.AlignBottom
            Layout.preferredWidth: btnSize
            Layout.preferredHeight: btnSize
            icon: "ios-navigate"
        }
    }
    /*MapToolBar {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: statusBar.top
        anchors.topMargin: 4
        anchors.leftMargin: 4
    }*/



    //scaleFactor: Screen.pixelDensity>0?Screen.pixelDensity/6:1

    //anchors.fill: parent
    //scale: 2

    /*MouseArea {
         anchors.fill: parent
         propagateComposedEvents: true
         //preventStealing: true
         onClicked: {
             console.log("map CLICKED")
             //mouse.accepted = false
         }
         onPressed: {
             console.log("map PRESSED")
             //mouse.accepted = false
         }
         onDoubleClicked: {
             console.log("map onDoubleClicked")
             map.zoomIn();
         }
    }*/



    /*MissionMenu {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.horizontalCenter
        anchors.bottom: statusBar.top
        //anchors.margins: 5

    }*/



    //Overlay items
    MapWind {
        id: wind
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 4
        anchors.rightMargin: 4
    }

    //top mission title bar
    Rectangle {
        id: titleRect
        anchors.top: parent.top
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        property double itemHeight: btnSize/2 //Math.max(Math.min(parent.width,parent.height)/50,Math.max(20,15*mapProvider.mapScaleFactor))
        width: titleText.width+itemHeight*6
        height: itemHeight*1.2
        border.width: 0
        color: "#A0000000"
        radius: height/2
        clip: true
        Text {
            id: titleText
            anchors.left: parent.left
            anchors.leftMargin: titleRect.itemHeight
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -1
            font.pixelSize: parent.height*0.8
            font.bold: true
            color: "white"
            text: mission.name
        }
        FastBlur {
            visible: mouseArea.containsMouse
            anchors.fill: titleText
            source: titleText
            radius: 8
        }
        /*Glow {
            visible: mouseArea.containsMouse
            anchors.fill: titleText
            radius: 3
            samples:6
            color: "#cfc"
            source: titleText
        }*/
        MouseArea {
            id: mouseArea
            anchors.fill: titleText
            enabled: true
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                console.log("click button");
                //buttonItem.clicked();
            }
        }
        Row {
            id: titleRow
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: titleRect.itemHeight/2
            //width: titleText.width+sz*(2+1)
            //anchors.verticalCenter: parent.verticalCenter
            //anchors.verticalCenterOffset: -1
            spacing: sz/2
            property int sz: titleRect.itemHeight*1.2

            MapButton {
                size: titleRow.sz
                //icons: "dropicons"
                icon: "android-sync"
                color: mission.modified?"#cfc":"#ccc"
                shadow: false
                //onClicked: mission.upload();
            }
            MapButton {
                size: titleRow.sz
                //icons: "dropicons"
                icon: "android-download"
                color: "#ccf"
                shadow: false
                onClicked: mission.request();
            }

        }
    }
    //mission quick toolbar
    /*Row {
        id: quickTools
        anchors.left: titleRect.right
        anchors.verticalCenter: titleRect.verticalCenter
        anchors.leftMargin: spacing
        spacing: sz/2
        property int sz: titleRect.itemHeight*1

        MapButton {
            size: quickTools.sz
            //icons: "dropicons"
            icon: "android-sync"
            color: "#cfc"
            //onClicked: mission.upload();
        }
        MapButton {
            size: quickTools.sz
            //icons: "dropicons"
            icon: "android-download"
            color: "#ccf"
            onClicked: mission.request();
        }

    }*/




    /*ListView {
        anchors.left: parent.left
        anchors.bottom: bottomInfo.top
        anchors.top: parent.top
        width: 50;
        //height: 200
        spacing: 5
        snapMode: ListView.SnapToItem
        flickableDirection: Flickable.VerticalFlick


        highlight: Rectangle { color: "lightsteelblue"; }
        focus: true
        populate: Transition {
            NumberAnimation { properties: "x,y"; duration: 100 }
        }


        model: mission.waypoints
        delegate: MapText {
            //color: "transparent"
            //opacity: 0.7
            text: "WP: " + index
        }
    }*/
    /*TreeView {
    }*/

    //Scrollbars
    /*Rectangle {
        id: scrollbarY
        anchors.right: parent.right
        y: map.visibleArea.yPosition * map.height
        width: 4
        height: Math.max(5,map.visibleArea.heightRatio * map.height)
        color: "white"
    }
    Rectangle {
        id: scrollbarX
        anchors.bottom: parent.bottom
        x: map.visibleArea.xPosition * map.width
        height: 4
        width: Math.max(5,map.visibleArea.widthRatio * map.width)
        color: "white"
    }*/
    Rectangle {
        id: centerMark
        anchors.centerIn: map
        width: 4*mapProvider.mapScaleFactor
        height: 4*mapProvider.mapScaleFactor
        color: "white"
        opacity: 0.5
    }
    Rectangle {
        id: trackingMark
        anchors.centerIn: centerMark
        visible: map.tracking
        width: 12*mapProvider.mapScaleFactor
        height: 12*mapProvider.mapScaleFactor
        border.width: 1*mapProvider.mapScaleFactor
        border.color: "#C000C000"
        color: "transparent"
    }


}
