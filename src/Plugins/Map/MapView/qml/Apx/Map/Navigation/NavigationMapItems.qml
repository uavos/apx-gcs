import QtQuick 2.12
import QtLocation 5.13

MapItemGroup {
    id: group
    z: 150

    Component.onCompleted: {
        map.addMapItemGroup(group)
    }


    TravelPath { }
    EnergyCircle { }
    CmdPosCircle { }
    Home { }
    LoiterCircle { }

    CamTargetCircleCmd { }
    CamTargetCircle { }

}
