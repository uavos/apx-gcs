import QtQuick 2.2
import com.uavos.map 1.0
import "."


Flickable {
    id: flick
    //anchors.fill: parent
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    contentWidth: (mapProvider.levelTiles)*mapProvider.tileSize
    contentHeight: (mapProvider.levelTiles)*mapProvider.tileSize
    boundsBehavior: Flickable.StopAtBounds
    pixelAligned: true
    maximumFlickVelocity: 2000*mapProvider.mapScaleFactor //Math.max(width,height)*2
    clip: true

    property bool optimizeShift: true
    property int animation_duration: 100
    property alias downloadCnt: mapTiles.downloadCnt

    signal deselect();

    signal flickToXYFinished();

    property rect contentRect: Qt.rect(flick.contentX,flick.contentY,flick.width,flick.height);
    onContentRectChanged: mapProvider.visibleArea=contentRect;

    onFlickingChanged: {
        //console.log("flicking: "+flicking);
    }

    //methods
    function jumpToLL(lat,lon){
        console.log("jumpToLL: "+lat+","+lon);
        jumpToXY(mapProvider.lonToX(lon),mapProvider.latToY(lat));
    }

    function jumpToXY(x,y){
        //console.log("jumpToXY: "+x+","+y);
        contentX=map.mapToSceneX(x)-width/2;
        contentY=map.mapToSceneY(y)-height/2;
    }

    function flickToXY(x,y){
        //console.log("flickToXY: "+x+","+y);
        flickToAnimationX.to=map.mapToSceneX(x)-width/2;
        flickToAnimationX.running=true;
        flickToAnimationY.to=map.mapToSceneY(y)-height/2;
        flickToAnimationY.running=true;
        //contentX=map.mapToSceneX(x)-width/2;
        //contentY=map.mapToSceneY(y)-height/2;
    }

    function deepDownload(){
        console.log("deepDownload...");
        mapTiles.deepDownload();
    }

    PropertyAnimation {
        id: flickToAnimationX
        running: false
        target: flick
        property: "contentX"
        duration: flick.animation_duration*4
        easing.type: Easing.OutQuart
        onStopped: if(!flickToAnimationY.running)flickToXYFinished();
    }
    PropertyAnimation {
        id: flickToAnimationY
        running: false
        target: flick
        property: "contentY"
        duration: flickToAnimationX.duration
        easing: flickToAnimationX.easing
        onStopped: if(!flickToAnimationX.running)flickToXYFinished();
    }

    //maintain center when window size changes
    property int ws: width
    property int hs: height
    onWidthChanged: {
        contentX-=(width-ws)/2;
        ws=width;
    }
    onHeightChanged: {
        contentY-=(height-hs)/2;
        hs=height;
    }



    //coordinates conversion
    property double constShiftX: -mapProvider.shift.x*mapProvider.tileSize
    property double constShiftY: -mapProvider.shift.y*mapProvider.tileSize
    property double constSceneXY: mapProvider.levelTiles*mapProvider.tileSize

    function levelToGlobal(v){ return v*mapProvider.maxTiles/mapProvider.levelTiles; }
    function globalToLevel(v){ return v*mapProvider.levelTiles/mapProvider.maxTiles; }

    function mapToSceneX(x){ return x*constSceneXY+constShiftX; }
    function mapToSceneY(y){ return y*constSceneXY+constShiftY; }

    function sceneToGlobalX(x){ return (x-constShiftX)/constSceneXY; }
    function sceneToGlobalY(y){ return (y-constShiftY)/constSceneXY; }

    function sceneToWindowX(x){ return (x-contentX)/width; }
    function sceneToWindowY(y){ return (y-contentY)/height; }

    //internal use
    property int minLevel: Math.floor(Math.sqrt(Math.max(width,height)/mapProvider.tileSize))+1
    property point defaultLL: Qt.point(47.2589912414551,11.3327512741089)

    onMinLevelChanged: if(mapProvider.level<minLevel)mapProvider.level=minLevel;
    Component.onCompleted: {
        flick.jumpToLL(flick.defaultLL.x,flick.defaultLL.y);
        initTimer.start();
    }
    Timer {
        id: initTimer
        interval: 1000; running: false; repeat: false
        onTriggered: {
            //delayed as window resizes
            flick.jumpToLL(flick.defaultLL.x,flick.defaultLL.y);
            mapTiles.provider=mapProvider
        }
    }

    //object tracking
    property point trackXY
    //property double contentX_s
    //property double contentY_s
    //Behavior on contentX_s { NumberAnimation { duration: 500; } }
    //Behavior on contentY_s { NumberAnimation { duration: 500; } }
    //onContentX_sChanged: flick.contentX=flick.contentX_s;
    //onContentY_sChanged: flick.contentY=flick.contentY_s;
    onTrackXYChanged: {
        //moveToXY(trackXY.x,trackXY.y);
        jumpToXY(trackXY.x,trackXY.y);
        //flick.contentX_s=map.mapToSceneX(flick.trackXY.x)-flick.width/2;
        //flick.contentY_s=map.mapToSceneY(flick.trackXY.y)-flick.height/2;
    }

    /*property point moveXY
    Behavior on moveXY {
        id: moveBehavior
        PropertyAnimation { duration: 10; }
    }
    onMoveXYChanged: {
        contentX=map.mapToSceneX(moveXY.x)-width/2;
        contentY=map.mapToSceneY(moveXY.y)-height/2;
    }
    function moveToXY(x,y){
        //moveBehavior.enabled=false;
        //moveXY=mapProvider.center;
        //moveBehavior.enabled=true;
        moveXY=Qt.point(x,y);
        //console.log("jumpToXY: "+x+","+y);
        //contentX=map.mapToSceneX(x)-width/2;
        //contentY=map.mapToSceneY(y)-height/2;
    }*/

    property bool tracking: false
    function startTracking(pointXY) {
        //console.log("startTracking"+trackXY);
        flick.trackXY=pointXY;
        flick.tracking=true;
        jumpToXY(trackXY.x,trackXY.y);
    }
    function stopTracking() {
        //if(!tracking)return;
        //console.log("stopTracking"+trackXY);
        trackXY=trackXY;
        tracking=false;
    }
    onDragStarted: {
        //console.log("onDragStarted");
        flick.stopTracking();
    }
    //onMovementEnded:




    //shift adjust
    function updateShift()
    {
        if(!flick.optimizeShift)return;
        //console.log("updateShiftImmediate");
        if(flick.zooming)return;
        var sX=0,sY=0;
        if(mapProvider.level>=10 && flick.optimizeShift){
            sX=Math.max(0,Math.floor((flick.contentX/mapProvider.tileSize+mapProvider.shift.x)/1000-1)*1000);
            sY=Math.max(0,Math.floor((flick.contentY/mapProvider.tileSize+mapProvider.shift.y)/1000-1)*1000);
        }
        var shiftXY_d=Qt.point(sX-mapProvider.shift.x,sY-mapProvider.shift.y);
        mapProvider.shift=Qt.point(sX,sY);
        //console.log("shiftXY_d: ("+shiftXY_d.x+","+shiftXY_d.y+")")
        flick.contentX-=shiftXY_d.x*mapProvider.tileSize;
        flick.contentY-=shiftXY_d.y*mapProvider.tileSize;
        flick.returnToBounds()
    }
    onMovementEnded: {
        updateShift();
        if(tracking)jumpToXY(trackXY.x,trackXY.y);
    }

    //Zoom
    property bool zooming
    property bool zoomingIn
    property double tmpScale: 1
    property point cScale
    property point cScaleMS
    Behavior on tmpScale {
        id: scaleAnim;
        animation: NumberAnimation {
            duration: 200
            onRunningChanged: {
                if(!running){
                    //console.log("scaleAnim stopped");
                    scaleAnim.enabled=false;
                    flick.tmpScale=1
                    mapProvider.level+=flick.zoomingIn?1:-1;
                    flick.zooming=false;
                    if(flick.tracking){
                        flick.cancelFlick();
                        //flick.trackingJump=true;
                        flick.jumpToXY(flick.cScale.x,flick.cScale.y);
                    }else{
                        //flick.jumpToXY(flick.cScale.x,flick.cScale.y);
                        flick.contentX=map.mapToSceneX(flick.cScale.x)-flick.cScaleMS.x;
                        flick.contentY=map.mapToSceneY(flick.cScale.y)-flick.cScaleMS.y;
                    }
                    flick.interactive=true;
                    flick.updateShift();
                    flick.returnToBounds()
                }
            }
        }
    }
    transform: Scale {
        property real oX: map.mapToSceneX(flick.cScale.x)-flick.contentX
        property real oY: map.mapToSceneY(flick.cScale.y)-flick.contentY
        origin.x: oX
        origin.y: oY
        xScale: flick.tmpScale;
        yScale: flick.tmpScale;
        //onOXChanged: console.log(oX);
    }
    //onTmpScaleChanged: flick.cancelFlick();
    function zoomIn(origin){flick.zoomAdjust(true,origin)}
    function zoomOut(origin){flick.zoomAdjust(false,origin)}
    function zoomAdjust(zoomIn,origin)
    {
        flick.cancelFlick();
        if(flick.zooming)return;
        //console.log("zoomAdjust");
        //console.log(zoomIn?"zoomIn":"zoomOut");
        flick.zoomingIn=zoomIn;
        if(zoomIn){
            if(mapProvider.level>=mapProvider.maxLevel)return;
        }else{
            if(mapProvider.level<=minLevel)return;
        }
        flick.zooming=true;
        if(flick.tracking || (!origin)){
            origin=mapProvider.center;
        }
        cScale=origin;
        cScaleMS=Qt.point(sceneToWindowX(mapToSceneX(origin.x))*width,sceneToWindowY(mapToSceneY(origin.y))*height);
        scaleAnim.enabled=true;
        flick.tmpScale*=zoomIn>0?2:0.5
    }

    pressDelay: 0
    MouseArea {
         id: mouseArea
         //parent: mapProvider
         anchors.fill: parent
         enabled: true
         hoverEnabled: true
         propagateComposedEvents: true
         preventStealing: false
         //z: 100
         onPositionChanged: {
             mapProvider.mousePos=Qt.point(mouseX,mouseY);
             mouse.accepted = false
         }
         onClicked: {
             console.log("CLICKED ON UPPER")
             //mouse.accepted = false
         }
         onDoubleClicked: {
             console.log("map onDoubleClicked")
             //mouse.accepted = false
             flick.zoomIn(mapProvider.mouse);
         }
         onPressAndHold: {
             //mouse.accepted = false;
         }
         onPressed: {
             console.log("PRESSED ON UPPER")
             //mouse.accepted = false
             flick.deselect();
         }
         onReleased: {
             console.log("RELEASED ON UPPER")
             //mouse.accepted = false
         }
         onWheel: {
             //mouse.accepted=true
             //wheel.accepted=true
             //console.log("WHEEL")
             if (wheel.modifiers & Qt.ControlModifier){
                 //flicking
                 //flick.stopTracking();
                 wheel.accepted=false
             }else{
                 wheel.accepted=true
                 if (wheel.angleDelta.y > 0) flick.zoomIn(mapProvider.mouse)
                 else flick.zoomOut(mapProvider.mouse)
             }
         }
    }

    /*PinchArea {
        width: flick.contentWidth
        height: flick.contentHeight

        property real initialWidth
        property real initialHeight
        onPinchStarted: {
            flick.interactive = false;
            //initialWidth = flick.contentWidth
            //initialHeight = flick.contentHeight
        }
        onPinchUpdated: {
            // adjust content pos due to drag
            //flick.contentX += pinch.previousCenter.x - pinch.center.x
            //flick.contentY += pinch.previousCenter.y - pinch.center.y
            // resize content
            //flick.resizeContent(initialWidth * pinch.scale, initialHeight * pinch.scale, pinch.center)
        }
        onPinchFinished: {
            flick.interactive = true;
            // Move its content within bounds.
            //flick.returnToBounds()
            console.log("onPinchFinished: "+pinch.scale);
        }
    }*/
    QmlMapTiles {
        id: mapTiles
        z: -1000000
    }

}
