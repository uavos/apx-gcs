import QtQuick 2.2
import "."
import "../"


Item {
    id: vehiclesItem
    Repeater {
        model: vehicles
        Vehicle {}
    }
}
