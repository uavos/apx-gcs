import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtGraphicalEffects 1.0

//import APX.Places 1.0
import Apx.Map 1.0
import Apx.Common 1.0


MapObject {
    id: control
    implicitZ: -100

    implicitCoordinate: QtPositioning.coordinate(modelData.lat,modelData.lon)
    title: modelData.title


    textColor: "white"
    color: Style.cGreen //"white"
    hoverScaleFactor: 1
    opacity: ui.effects?((hover||selected)?1:0.7):1

    onMenuRequested: {
        apx.tools.sites.edit.requestMenu({"closeOnActionTrigger":true, "pos":Qt.point(0,ui.window.height)})
    }

    //dragging support
    onMovingFinished: {
        if(selected){
            var d=modelData
            d.lat=coordinate.latitude
            d.lon=coordinate.longitude
            apx.tools.sites.lookup.dbModel.set(index,d)
        }
    }
    draggable: selected

    onSelectedChanged: {
        if(selected){
            apx.tools.sites.createEditor(modelData)
        }else{
            apx.tools.sites.destroyEditor(modelData)
        }
    }
}
