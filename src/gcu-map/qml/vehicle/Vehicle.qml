import QtQuick 2.2
import "."
import "../components"


Item {
    id: vehicleItem
    z: 100
    //local mandala item (collect all fields)
    property var m: vehicles[index]
    property var f_rc_roll: m.field("rc_roll")
    property var f_rc_pitch: m.field("rc_pitch")
    property var f_lat: m.field("gps_lat")
    property var f_lon: m.field("gps_lon")
    property var f_yaw: m.field("yaw")
    property var f_roll: m.field("roll")
    property var f_pitch: m.field("pitch")

    property var f_home_lat: m.field("home_lat")
    property var f_home_lon: m.field("home_lon")

    //internal [levelXY 0..1]
    property double px: mapProvider.lonToX(f_lon.value)
    property double py: mapProvider.latToY(f_lat.value)
    property point pos: mapProvider.center //initial pos
    Behavior on pos {
        PropertyAnimation {
            id: posAnim
            duration: 500;// map.animation_duration;
            onToChanged: {
                if(Math.abs(mapProvider.mapToScene(to.x-pos.x))>map.width){
                    //console.log("Complete x");
                    complete();
                }else if(Math.abs(mapProvider.mapToScene(to.y-pos.y))>map.height){
                    //console.log("Complete x");
                    complete();
                }
            }
        }
    }

    /*onPxChanged: {
        if(Math.abs(map.sceneX(px-pos.x))>map.width){
            //console.log("Complete x");
            posAnim.complete();
        }else if(Math.abs(map.sceneY(py-pos.y))>map.height){
            //console.log("Complete y");
            posAnim.complete();
        }
    }*/

    Component.onCompleted: {
        console.log("Vehicle #"+index);
        vehicleItem.pos=Qt.binding(function(){return Qt.point(vehicleItem.px,vehicleItem.py)});
        //if(index==0)follow();
    }
    function follow()
    {
        //console.log("Vehicle #"+index+" tracking");
        map.startTracking(Qt.binding(function(){return vehicleItem.pos}));
    }

    CmdCircle {
        parent: vehicleItem.parent
        //m: vehicleItem.m
    }
    Item{
        x: map.mapToSceneX(vehicleItem.pos.x) //-width/2
        y: map.mapToSceneY(vehicleItem.pos.y) //-height/2
        scale: 1/map.tmpScale //zoom no scale
        Image {
            id: icon
            source: "/icons/uav.svg"
            anchors.centerIn: parent
            sourceSize.width: 48*mapProvider.mapScaleFactor
            sourceSize.height: width

            transform: [
                Scale {
                    origin.x: icon.width/2
                    origin.y: icon.height/2
                    xScale: 1/(0.05*mapProvider.mapScaleFactor);
                    yScale: xScale;
                },
                Rotation {
                    origin.x: icon.width/2
                    origin.y: icon.height/2
                    axis.y: 1
                    axis.z: 0
                    angle: f_roll.value
                    Behavior on angle { RotationAnimation {duration: map.animation_duration; direction: RotationAnimation.Shortest; } }
                },
                Rotation {
                    origin.x: icon.width/2
                    origin.y: icon.height/2
                    axis.x: 1
                    axis.z: 0
                    angle: -f_pitch.value
                    Behavior on angle { RotationAnimation {duration: map.animation_duration; direction: RotationAnimation.Shortest; } }
                },
                Scale {
                    origin.x: icon.width/2
                    origin.y: icon.height/2
                    xScale: 0.05*mapProvider.mapScaleFactor;
                    yScale: xScale;
                },
                Rotation {
                    origin.x: icon.width/2
                    origin.y: icon.height/2
                    axis.z: 1
                    angle: f_yaw.value
                    Behavior on angle { RotationAnimation {duration: map.animation_duration; direction: RotationAnimation.Shortest; } }
                }
            ]

            MouseArea {
                anchors.fill: parent
                onClicked: vehicleItem.follow();
            }
        }
        MapText {
            anchors.top: icon.bottom
            anchors.horizontalCenter: icon.horizontalCenter
            color: Style.cGreen
            textColor: "white"
            //scale: 1/mapProvider.mapScaleFactor
            text: index+" ("+m.xpdrData+")"
                  //+" ("+f_lat.value+","+f_lon.value+")\n"
        }
    }
}
