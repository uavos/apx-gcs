import QtQuick 2.5
//import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import ".."

MapText {
    id: vehicleInfo
    property var vehicle: modelData
    property bool interactive: false

    //Fact bindings
    property var vm: vehicle.mandala
    property real altitude: vm.altitude.value
    property real vspeed: vm.vspeed.value
    property string modeText: vm.mode.text
    property int stage: vm.stage.value
    property bool active: vehicle.active

    property bool bGCU: vehicle.vclass.value===Vehicle.GCU
    property bool bLOCAL: vehicle.vclass.value===Vehicle.LOCAL

    opacity: (interactive && (!active) && (!mouseArea.containsMouse))?0.4:1
    Behavior on opacity { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }

    text: bGCU?vehicle.callsign.value:(
            (bLOCAL?"":(vehicle.callsign.value+"\n"))+
            "H"+(altitude>50?(altitude/10).toFixed()+"0":Math.abs(altitude)<1?"0":altitude.toFixed())+
            (vspeed>1?" +"+vspeed.toFixed():vspeed<-1?" "+vspeed.toFixed():"")+"\n"+
            modeText + (stage>1?"/"+stage:"")
        )

    color: bGCU?"#3779C5":
        vehicle.stream.value===Vehicle.TELEMETRY?"#377964":
        vehicle.stream.value===Vehicle.XPDR?"#376479":"gray"


    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: interactive
        hoverEnabled: enabled
        cursorShape: enabled?Qt.PointingHandCursor:Qt.ArrowCursor
        onClicked: {
            if(active){
                map.centerOnCoordinate(QtPositioning.coordinate(vm.gps_lat.value,vm.gps_lon.value))
            }else{
                app.vehicles.selectVehicle(vehicle);
            }
        }

    }
}
