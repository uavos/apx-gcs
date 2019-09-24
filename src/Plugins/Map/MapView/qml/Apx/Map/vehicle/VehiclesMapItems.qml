import QtQuick          2.3
import QtLocation       5.3
import QtPositioning    5.3

MapItemView {
    model: apx.vehicles.list.model
    delegate: VehicleItem {  }
}

