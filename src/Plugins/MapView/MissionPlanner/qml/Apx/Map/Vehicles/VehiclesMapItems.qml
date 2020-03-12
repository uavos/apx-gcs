import QtQuick 2.12
import QtLocation 5.13

MapItemGroup {
    id: group
    z: 200

    Component.onCompleted: {
        map.addMapItemGroup(group)
    }

    MapItemView {
        id: vehicles
        model: apx.vehicles.model
        delegate: VehicleItem { }
    }
}
