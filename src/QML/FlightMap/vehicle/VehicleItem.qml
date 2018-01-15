import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import "."
import "../mission"

MapQuickItem {  //to be used inside MapComponent only
    id: vehicleItem

    property Vehicle vehicle: modelData

    //Fact bindings
    property var vm: vehicle.mandala
    property real roll: vm.roll.value
    property real pitch: vm.pitch.value
    property real yaw: vm.yaw.value
    property real lat: vm.gps_lat.value
    property real lon: vm.gps_lon.value
    property real altitude: vm.altitude.value
    property bool active: vehicle.active
    property real cmd_course: vm.cmd_course.value
    property real course: vm.course.value
    property real vspeed: vm.vspeed.value
    property string modeText: vm.mode.text
    property int stage: vm.stage.value

    property bool bGCU: vehicle.vclass.value===Vehicle.GCU
    property bool bLOCAL: vehicle.vclass.value===Vehicle.LOCAL

    //position
    coordinate: QtPositioning.coordinate(lat,lon,altitude)

    //smooth move
    property bool smoothMove: false
    Behavior on coordinate { enabled: app.settings.smooth.value && smoothMove; CoordinateAnimation {duration: 500; direction: CoordinateAnimation.Shortest; } }
    property variant coordChk: QtPositioning.coordinate(lat,lon)
    onCoordChkChanged: {
        if(!smoothMove){
            if(coordChk.latitude!==0 && coordChk.longitude!==0){
                smoothMove=true;
                coordChk=0;
            }
        }
    }

    //follow
    property bool isFollow: map.itemToFollow===vehicleItem && map.follow
    onActiveChanged: {
        if(active){
            if(coordinate!==QtPositioning.coordinate(0,0)){
                map.centerOnItem(vehicleItem)
            }else{

            }
        }else if(isFollow) map.followStop()
    }

    //animated vars
    property real vyaw: yaw
    Behavior on vyaw { enabled: app.settings.smooth.value; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }


    //info box
    property bool bInfoShowRight: true
    property bool bInfoPosChk2: yaw<=50 || yaw>120
    property bool bInfoPosChk1: yaw<=40 || yaw>130
    onBInfoPosChk1Changed: {
        if(bGCU)return;
        if(bInfoShowRight)return;
        if(bInfoPosChk1)bInfoShowRight=true
    }
    onBInfoPosChk2Changed: {
        if(bGCU)return;
        if(!bInfoShowRight)return;
        if(!bInfoPosChk2)bInfoShowRight=false
    }


    //constants
    property int animation_duration: 100

    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2

    sourceItem:
    Item {
        Image {
            id: image
            source: bGCU?"../icons/gcu.svg":"../icons/uav.svg"
            sourceSize.width: (bGCU?32:48)*map.itemsScaleFactor
            sourceSize.height: width

            opacity: active?0.9:0.7
            scale: active?1:0.6

            transform: [
                Scale {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    xScale: 1/(0.05*map.itemsScaleFactor);
                    yScale: xScale;
                },
                Rotation {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    axis.y: 1
                    axis.z: 0
                    angle: roll
                    Behavior on angle { enabled: app.settings.smooth.value; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
                },
                Rotation {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    axis.x: 1
                    axis.z: 0
                    angle: -pitch
                    Behavior on angle { enabled: app.settings.smooth.value; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
                },
                Scale {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    xScale: 0.05*map.itemsScaleFactor;
                    yScale: xScale;
                },
                Rotation {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    axis.z: 1
                    angle: vyaw-map.bearing
                }
            ]

        }
        Image {
            id: cmdCrsArrow
            source: "../icons/cmd-crs-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            smooth: true
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: cmdCrsArrow.width/2
                origin.y: cmdCrsArrow.height
                property real v: cmd_course
                angle: v-map.bearing
                Behavior on v { enabled: app.settings.smooth.value; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }
            //opacity: vehicle.active?1:0.8
        }
        Image {
            id: crsArrow
            source: "../icons/crs-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            smooth: true
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: crsArrow.width/2
                origin.y: crsArrow.height
                property real v: course
                angle: v-map.bearing
                Behavior on v { enabled: app.settings.smooth.value; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }
            //opacity: vehicle.active?1:0.8
        }
        Image {
            id: hdgArrow
            source: "../icons/hdg-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            smooth: true
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: crsArrow.width/2
                origin.y: crsArrow.height
                angle: vyaw-map.bearing
            }
            //opacity: vehicle.active?1:0.8
        }
        //follow map circle
        Rectangle {
            border.color: "#222"
            border.width: 2
            color: "transparent"
            width: image.width*0.3
            height: width
            anchors.centerIn: image
            visible: isFollow
            radius: width/2
        }
        //info label
        VehicleInfo {
            z: image.z-5
            vehicle: vehicleItem.vehicle
            opacity: 0.8
            anchors.centerIn: image
            anchors.verticalCenterOffset: bInfoShowRight?0:(image.height/2+height/2)
            anchors.horizontalCenterOffset: bInfoShowRight?(image.width/2+width/2):0
        }

        MouseArea {
            anchors.fill: image
            drag.target: vehicleItem
            onClicked: {
                if(!active) app.vehicles.selectVehicle(vehicle);
                else if(isFollow) map.followStop()
                else map.followItem(vehicleItem)
            }
        }

        transform: [
            Rotation {
                origin.x: image.width/2
                origin.y: image.height/2
                axis.x: 1
                axis.z: 0
                angle: map.tilt
            }
        ]
    }

    //Mission
    Component.onCompleted: {
        var c=missionC.createObject(map)
        map.addMapItemGroup(c)
        vehicle.removed.connect(function(){c.destroy()})
    }
    Component {
        id: missionC
        MissionItem {
            vehicle: vehicleItem.vehicle
        }
    }

}
