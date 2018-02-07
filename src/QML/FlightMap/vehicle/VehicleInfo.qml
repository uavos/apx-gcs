import QtQuick 2.5
import QtQuick.Layouts 1.3
//import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import ".."

MapButton {
    id: vehicleInfo
    property var vehicle: modelData

    property var menuFact: vehicle

    //Fact bindings
    property var vm: vehicle.mandala
    property real altitude: vm.altitude.value
    property real vspeed: vm.vspeed.value
    property string modeText: vm.mode.text
    property int stage: vm.stage.value
    active: vehicle.active
    visible: vehicle.visible

    property bool bGCU: vehicle.vehicleClass===Vehicle.GCU
    property bool bLOCAL: vehicle.vehicleClass===Vehicle.LOCAL


    //internal
    enabled: false
    defaultOpacity: 0.6
    rightMargin: 10

    property string callsign: vehicle.callsign

    text: bGCU?callsign:(
            callsign+"\n"+ //(bLOCAL?"":(callsign+"\n"))+
            "H"+(altitude>50?(altitude/10).toFixed()+"0":Math.abs(altitude)<1?"0":altitude.toFixed())+
            (vspeed>1?" +"+vspeed.toFixed():vspeed<-1?" "+vspeed.toFixed():"")+"\n"+
            modeText + (stage>1?"/"+stage:"")
        )

    color: bGCU?"#3779C5":
        vehicle.streamType===Vehicle.TELEMETRY?"#377964":
        vehicle.streamType===Vehicle.XPDR?"#376479":"gray"


    onClicked: {
        if(active){
            map.centerOnCoordinate(QtPositioning.coordinate(vm.gps_lat.value,vm.gps_lon.value))
        }else{
            app.vehicles.selectVehicle(vehicle);
        }
    }

    onMenuRequested: map.showFactMenu(menuFact)


    //right side info
    ColumnLayout {
        anchors.right: parent.right
        anchors.top: parent.top
        //anchors.bottom: parent.bottom
        anchors.margins: 1
        spacing: 3

        //recording red point
        Rectangle {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            visible: vehicle.recorder.recording
            border.width: 0
            width: 8
            height: width
            radius: width/2
            color: "#C0FF8080"
        }
        //mission available
        Rectangle {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            visible: vehicle.mission.missionSize>0
            border.width: 0
            width: 8
            height: width
            radius: width/2
            color: "#C080FFFF"
        }
    }
}
