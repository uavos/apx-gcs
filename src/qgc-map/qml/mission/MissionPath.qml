import QtQuick 2.2
import com.uavos.map 1.0
import "."


Canvas {
    id: canvas
    parent: map
    anchors.fill: parent
    canvasSize.width: mapProvider.levelTiles*mapProvider.tileSize//flickable.contentWidth
    canvasSize.height: mapProvider.levelTiles*mapProvider.tileSize//flickable.contentHeight
    tileSize.width: mapProvider.tileSize
    tileSize.height: mapProvider.tileSize
    canvasWindow.x: (map.contentX+map.shiftXY.x*mapProvider.tileSize)//.toFixed()
    canvasWindow.y: (map.contentY+map.shiftXY.y*mapProvider.tileSize)//.toFixed()
    canvasWindow.width: map.width
    canvasWindow.height: map.height
    renderStrategy: Canvas.Immediate//Threaded//Immediate //Threaded //.Cooperative
    renderTarget: Canvas.Image//Image //.FramebufferObject
    contextType: "2d"

    onPaint: {
        context.clearRect(region.x,region.y,region.width,region.height);

        //console.log(region+" m: "+flickable.mX*mapProvider.tileSize+","+flickable.mY*mapProvider.tileSize);
    }
    Component.onCompleted: canvas.requestPaint();
    onCanvasWindowChanged: canvas.requestPaint();
}
