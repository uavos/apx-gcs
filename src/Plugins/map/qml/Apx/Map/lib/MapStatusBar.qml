import QtQuick 2.2

Rectangle {
    id: mapStatusBar
    property int fontSize: height-2
    property font textFont: font_narrow

    //bottom info
    border.width: 0
    color: "#A0000000"
    Row {
        spacing: 4
        Text {
            font.family: textFont
            font.pixelSize: fontSize
            text: apx.latToString(map.mouseCoordinate.latitude)+apx.lonToString(map.mouseCoordinate.longitude)
            color: "white"
        }
        Text {
            font.family: textFont
            font.pixelSize: fontSize
            text: "L"+map.zoomLevel.toFixed()
            color: "white"
        }
        /*Text {
            visible: map.downloadCnt>0
            font.family: textFont
            font.pixelSize: fontSize
            text: "D"+map.downloadCnt
            color: "white"
        }
        Text {
            font.family: textFont
            font.pixelSize: fontSize
            visible: map.tracking
            text: "TRACK"
            color: "white"
        }*/
        /*Text {
            font.family: textFont
            font.pixelSize: fontSize
            visible: map.flicking
            text: "FLICK"
            color: "white"
        }
        Text {
            font.family: textFont
            font.pixelSize: fontSize
            visible: map.moving
            text: "MOVING"
            color: "white"
        }*/
        /*Text {
            font.family: textFont
            font.pixelSize: fontSize
            //property double mX: (map.mouse.x-map.center.x)*mapProvider.xToMeters(map.center.y)
            //property double mY: (map.mouse.y-map.center.y)*mapProvider.xToMeters(map.center.y)
            text:
                "content: ("+mapProvider.visibleArea.x+","+mapProvider.visibleArea.y+") "+
                //"c: ("+map.center.y.toFixed(2)+","+map.center.x.toFixed(2)+") "+
                //"cLL: ("+mapProvider.yToLat(map.center.y).toFixed(2)+","+mapProvider.xToLon(map.center.x).toFixed(2)+") "+
                //"m: ("+map.mouseX.toFixed(1)+","+map.mouseY.toFixed(1)+") "+
                //"content: ("+map.contentX.toFixed(1)+","+map.contentY.toFixed(1)+") "+
                //"cnt: "+mapProvider.size+" "+
                //"L:"+mapProvider.level+" "+
                //"icon: ("+map.uavPos.x.toFixed(1)+","+map.uavPos.y.toFixed(1)+") "+
                //"meters: "+mX.toFixed()+" "+
                ""
            color: "white"
        }*/
    }
}

