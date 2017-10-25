import QtQuick 2.2
import "."
import "../"
import "../components"
import com.uavos.map 1.0


MapObject {
    id: taxiwayItem
    mItem: model.modelData
    parent: missionItem

    color: Style.cNormal
    textColor: "white"

    property bool current: m.twidx.value === (index-1)
    property bool taxiing: m.mode.value === mode_TAXI

    property bool showDetails: mapProvider.level>13 || mItem.distance*mapProvider.metersToX(gy)*map.constSceneXY>height

    QmlMapPath {
        z: -110
        parent: missionItem
        provider: mapProvider
        lineWidth: (taxiing&&current)?6:3
        color: taxiing?(current?Style.cLineGreen:"yellow"):Style.cNormal
        opacity: 0.8
        path: mItem.path
        shift: Qt.point(-map.constShiftX,-map.constShiftY)
        visible: index>0 && showDetails
    }
}
