import QtQuick 2.5
//import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import ".."

MapButton {
    id: vehicleInfo
    property var vehicle: modelData

    //Fact bindings
    property var vm: vehicle.mandala
    property real altitude: vm.altitude.value
    property real vspeed: vm.vspeed.value
    property string modeText: vm.mode.text
    property int stage: vm.stage.value
    active: vehicle.active

    property bool bGCU: vehicle.vclass.value===Vehicle.GCU
    property bool bLOCAL: vehicle.vclass.value===Vehicle.LOCAL


    //internal
    enabled: false
    defaultOpacity: 0.6
    rightMargin: 10

    property string callsign: vehicle.callsign.value

    text: bGCU?callsign:(
            (bLOCAL?"":(callsign+"\n"))+
            "H"+(altitude>50?(altitude/10).toFixed()+"0":Math.abs(altitude)<1?"0":altitude.toFixed())+
            (vspeed>1?" +"+vspeed.toFixed():vspeed<-1?" "+vspeed.toFixed():"")+"\n"+
            modeText + (stage>1?"/"+stage:"")
        )

    color: bGCU?"#3779C5":
        vehicle.stream.value===Vehicle.TELEMETRY?"#377964":
        vehicle.stream.value===Vehicle.XPDR?"#376479":"gray"


    onClicked: {
        if(active){
            map.centerOnCoordinate(QtPositioning.coordinate(vm.gps_lat.value,vm.gps_lon.value))
        }else{
            app.vehicles.selectVehicle(vehicle);
        }
    }

    onMenuRequested: map.showFactMenu(vehicle)


    //recording red point
    Rectangle {
        visible: vehicle.recorder.recording
        border.width: 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 1
        width: 8
        height: width
        radius: width/2
        color: "#C0FF8080"
    }
}
