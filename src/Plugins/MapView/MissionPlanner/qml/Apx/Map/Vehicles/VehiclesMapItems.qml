import QtQuick 2.12
import QtLocation 5.13

MapItemGroup {
    id: group
    z: 200

    Component.onCompleted: {
        map.addMapItemGroup(group)
    }

    VehicleItem {
        vehicle: apx.vehicles.local
    }

    VehicleItem {
        vehicle: apx.vehicles.replay
    }

    MapItemView {
        id: vehicles
        model: apx.vehicles.list.model
        delegate: VehicleItem { }
    }
}
