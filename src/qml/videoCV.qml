import QtQuick 2.0
import QtMultimedia 5.0
//import Qt.labs.settings 1.0
import QtQuick.Controls 1.1
//import QtQuick.Dialogs 1.2
import FfmpegPlayer 0.1
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
    property bool online: mediaPlayer.error === MediaPlayer.NoError && mediaPlayer.playbackState === MediaPlayer.PlayingState
    property int retry: 0
    property bool flip: settings.value("flip",false)
    property bool autoRotate: settings.value("autoRotate",false)
    property bool autoScale: settings.value("autoScale",false)

    signal config

    onSrcChanged: settings.setValue("src",src);
    onFlipChanged: settings.setValue("flip",flip);
    onAutoRotateChanged: settings.setValue("autoRotate",autoRotate);
    onAutoScaleChanged: settings.setValue("autoScale",autoScale);

    function connect() {
        src = textSrc.text ? textSrc.text : src_default
        mediaPlayer.stop()
        mediaPlayer.source=""
        mediaPlayer.source=src
        mediaPlayer.play()
    }
    function disconnect() {
        retry = 0
        mediaPlayer.source=""
        mediaPlayer.stop()
        mediaPlayer.source=src
    }

    VideoOutput {
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        source: mediaPlayer

        rotation: app.settings.smooth.value_angle((flip?180:0)+(autoRotate?(cam_roll.value+roll.value):0),"videoOutput")
        Behavior on rotation { PropertyAnimation {duration: app.settings.smooth.value?200:0} }
        scale: 1.0+(autoScale?Math.abs(Math.sin(rotation*3.14/180))*width/height:0)
    }

    FfmpegPlayer {
        id: mediaPlayer;
        source: src
    }

    BusyIndicator {
        id: busyIndicator
        running: true
        anchors.centerIn: parent
        visible: mediaPlayer.connectingState
    }

    Button {
        id: cancelButton
        anchors.topMargin: 10
        anchors.top: busyIndicator.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 24
        onClicked: {
            disconnect()
        }
        text: "Cancel"
        visible: mediaPlayer.connectingState
    }


    Item{
        id: connectItem
        anchors.fill: parent
        anchors.margins: 1
        visible: !online && !mediaPlayer.connectingState
        Column {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            Text{
                id: textErr
                anchors.horizontalCenter: parent.horizontalCenter
                color: "yellow"
                wrapMode: Text.WordWrap
                text: mediaPlayer.errorString
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
                text: mediaPlayer.source
                onAccepted: {
                    retry=0
                    focus=false
                    connect()
                }

                onVisibleChanged: focus=false
                onFocusChanged: if(activeFocus)retry=0;
                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }
        }
        Button {
            anchors.centerIn: parent
            text: "connect ffmpeg"
            height: 24
            onClicked: {
                retry=5

                connect()
            }
        }
        Timer {
            id: reconnectTimer
            interval: 10000
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
            Button { text: "POI"; }
            Button { text: "CMD"; }
            Button { text: "ZOOM"; }
            Item { width: 20; height:1 }
            Button { text: "FL"; toolTip: "Flip image"; option: flip; onClicked: flip=!flip; }
            Button { text: "AR"; toolTip: "Auto rotate"; option: autoRotate; onClicked: autoRotate=!autoRotate; }
            Button { text: "AS"; toolTip: "Auto scale"; option: autoScale;onClicked: autoScale=!autoScale;  }

        }
        Row {
            anchors.top: parent.top
            anchors.right: parent.right
            spacing: 8
            Button {
                colorBG: "#ff478f"
                text: "OFF";
                onClicked: disconnect()
            }
        }
    }
}
