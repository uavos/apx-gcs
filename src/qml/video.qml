import QtQuick 2.0
import QtMultimedia 5.0
//import Qt.labs.settings 1.0
import QtQuick.Controls 1.1
//import QtQuick.Dialogs 1.2
import "./components"
/*
gst-launch rtspsrc location=rtspt://192.168.0.50/cam0_0 ! rtph264depay ! h264parse ! capsfilter caps="video/x-h264,width=1280,height=800,framerate=(fraction)25/1" ! ffdec_h264 ! ffmpegcolorspace ! autovideosink
ffmpeg -rtsp_transport tcp -i rtsp://192.168.0.50:554/cam0_0 -b 900k -vcodec copy -r 60 -y MyVdeoFFmpeg.avi
-------------------
sample stream:
rtspt://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov
rtspt://streaming1.osu.edu/media2/ufsap/ufsap.mov
-------------------
*/

Item {
    id: videoContainer
    property string src_default: "rtspt://127.0.0.1:554" //"rtspt://192.168.0.50:554/cam0_0"//"rtsp://192.168.0.50:554/cam0_0"//"rtsp://127.0.0.1:9121" //"v4l2:///dev/video0" //"v4l2:///dev/video0" //"rtsp://127.0.0.1:9121" //"v4l2:///dev/video0" //
    property string src: settings.value("src",src_default)
    property bool online: mediaplayer.error===MediaPlayer.NoError && mediaplayer.playbackState==MediaPlayer.PlayingState
    property int retry: 0
    property bool flip: settings.value("flip",false)
    property bool autoRotate: settings.value("autoRotate",false)
    property bool autoScale: settings.value("autoScale",false)

    signal config

    onSrcChanged: settings.setValue("src",src);
    onFlipChanged: settings.setValue("flip",flip);
    onAutoRotateChanged: settings.setValue("autoRotate",autoRotate);
    onAutoScaleChanged: settings.setValue("autoScale",autoScale);


    /*Settings {
        category: "video"
        property alias src: videoContainer.src
        property alias flip: videoContainer.flip
        property alias autoRotate: videoContainer.autoRotate
        property alias autoScale: videoContainer.autoScale
    }*/

    function connect() {
        mediaplayer.stop()
        mediaplayer.source=""
        mediaplayer.source=src
        mediaplayer.play()
        //mediaplayer.autoPlay=true
    }
    /*function do_config() {
        retry=0
        config()
        configDialog.open()
    }
    Dialog {
        id: configDialog
        visible: false
        title: "Choose a source"
        standardBtns: StandardBtn.Save | StandardBtn.Cancel

        onAccepted: connect()
        TextInput {
            focus: true
            font.pixelSize: 20
            color: "white"
            cursorVisible: true

            text: videoContainer.src
        }

    }*/

    VideoOutput {
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop //PreserveAspectFit
        source: mediaplayer

        rotation: mandala.angle((flip?180:0)+(autoRotate?(cam_roll.value+roll.value):0))
        Behavior on rotation { RotationAnimation {duration: app.settings.smooth.value?200:0; direction: RotationAnimation.Shortest; } }
        scale: 1.0+(autoScale?Math.abs(Math.sin(rotation*3.14/180))*width/height:0)
    }

    MediaPlayer {
        id: mediaplayer
        source: src
        muted: true
        autoLoad: false
        autoPlay: false
        //onStopped: play()
        onError: console.log(errorString)
    }

    /*Text{
        anchors.fill: parent
        color: "white"
        //verticalAlignment: Text.AlignVCenter
        //horizontalAlignment: Text.AlignHCenter
        text: (mediaplayer.status)
        visible: true //mediaplayer.status==MediaPlayer.Buffering
    }*/

    Item{
        id: connectItem
        anchors.fill: parent
        anchors.margins: 1
        visible: !online
        Column {
            anchors.bottom: parent.bottom
            //anchors.horizontalCenter: parent.horizontalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            Text{
                id: textErr
                anchors.horizontalCenter: parent.horizontalCenter
                color: "yellow"
                wrapMode: Text.WordWrap
                text: mediaplayer.errorString
            }
            TextInput {
                id: textSrc
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: TextInput.AlignHCenter
                color: (activeFocus||mouse.containsMouse)?"white":"yellow"
                font.pointSize: mandala.limit(parent.width/50,textErr.font.pointSize,25)
                wrapMode: Text.WordWrap
                selectByMouse: true
                activeFocusOnPress: true
                text: mediaplayer.source
                onAccepted: {
                    retry=0
                    focus=false
                    src=text?text:src_default
                    connect()
                }
                onVisibleChanged: focus=false
                onFocusChanged: if(activeFocus)retry=0;
                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    //cursorShape: Qt.PointingHandCursor;
                    acceptedButtons: Qt.NoButton
                }
            }
        }
        Btn {
            anchors.centerIn: parent
            text: qsTr("connect")
            height: 24
            onClicked: {
                retry=5
                connect()
            }
        }
        /*Btn {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            colorBG: "#88c"
            text: "CFG";
            onClicked: do_config()
        }*/
        Timer {
            id: reconnectTimer
            interval: 2500
            repeat: true
            running: retry>0 && (!online)
            onTriggered: {
                if(retry>0)retry--
                connect()
            }
        }
    }

    Item {
        id: buttons
        anchors.bottom: parent.bottom
        anchors.margins: 1
        width: parent.width
        height: row.height
        visible: online

        Row {
            id: row
            anchors.top: parent.top
            anchors.left: parent.left
            spacing: 2
            Btn { text: qsTr("POI"); }
            Btn { text: qsTr("CMD"); }
            Btn { text: qsTr("ZOOM"); }
            Item { width: 20; height:1 }
            Btn { text: qsTr("FL"); toolTip: qsTr("Flip image"); option: flip; onClicked: flip=!flip; }
            Btn { text: qsTr("AR"); toolTip: qsTr("Auto rotate"); option: autoRotate; onClicked: autoRotate=!autoRotate; }
            Btn { text: qsTr("AS"); toolTip: qsTr("Auto scale"); option: autoScale;onClicked: autoScale=!autoScale;  }

        }
        Row {
            anchors.top: parent.top
            anchors.right: parent.right
            spacing: 8
            /*Btn {
                colorBG: "#88c"
                text: "CFG";
                onClicked: do_config()
            }*/
            Btn {
                colorBG: "#ff478f"
                text: qsTr("OFF");
                onClicked: {
                    retry=0
                    mediaplayer.source=""
                    mediaplayer.stop()
                    mediaplayer.source=src
                }
            }
        }
    }

    /*MouseArea {
        anchors.fill: rootItem
        onClicked: {
        }
    }*/

}
