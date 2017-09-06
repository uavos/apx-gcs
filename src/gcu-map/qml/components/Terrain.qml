import QtQuick 2.2
import com.uavos.map 1.0
import "."


Flickable {
    id: flickable
    anchors.fill: parent
    property bool optimizeShift: false
    property int animation_duration: 100
    property double scaleFactor: 1 //interactive items scale factor
    property alias downloadCnt: mapTiles.downloadCnt

    signal deselect();

    onFlickingChanged: {
        //console.log("flicking: "+flicking);
    }
    property rect window: Qt.rect(visibleArea.xPosition,visibleArea.yPosition,visibleArea.widthRatio,visibleArea.heightRatio);
    //onVisibleAreaChanged: mapProvider.visibleArea=
    onWindowChanged: mapProvider.visibleArea=window


    //methods
    function jumpToLL(lat,lon){
        console.log("jumpToLL: "+lat+","+lon);
        jumpToXY(mapProvider.lonToX(lon),mapProvider.latToY(lat));
    }
    function jumpToXY(x,y){
        console.log("jumpToXY: "+x+","+y);
        contentX=flickable.sceneX(x)-width/2;
        contentY=flickable.sceneY(y)-height/2;
    }

    //coordinates conversion
    property double constShiftX: -shiftXY.x*mapProvider.tileSize
    property double constShiftY: -shiftXY.y*mapProvider.tileSize
    property double constSceneXY: mapProvider.levelTiles*mapProvider.tileSize

    function levelToGlobal(v){ return v*mapProvider.maxTiles/mapProvider.levelTiles; }
    function globalToLevel(v){ return v*mapProvider.levelTiles/mapProvider.maxTiles; }

    function sceneX(x){ return x*constSceneXY+constShiftX; }
    function sceneY(y){ return y*constSceneXY+constShiftY; }

    function sceneToGlobalX(x){ return (x-constShiftX)/constSceneXY; }
    function sceneToGlobalY(y){ return (y-constShiftY)/constSceneXY; }

    function sceneToWindowX(x){ return (x-contentX)/width; }
    function sceneToWindowY(y){ return (y-contentY)/height; }

    //internal use
    property int minLevel: Math.floor(Math.sqrt(Math.max(width,height)/mapProvider.tileSize))+1
    property int tilesCntMargin: 4
    property point defaultLL: Qt.point(47.2589912414551,11.3327512741089)
    property point tilesCnt: Qt.point(Math.floor(Math.max(3,width/mapProvider.tileSize+tilesCntMargin)),Math.floor(Math.max(3,height/mapProvider.tileSize+tilesCntMargin)))

    onMinLevelChanged: if(mapProvider.level<minLevel)mapProvider.level=minLevel;
    onTilesCntChanged: mapProvider.tilesCnt=tilesCnt
    Component.onCompleted: {
        mapProvider.tilesCnt=tilesCnt;
        contentX=flickable.contentWidth/2-400
        contentY=flickable.contentHeight/2-300
        initTimer.start();
    }
    Timer {
        id: initTimer
        interval: 1000; running: false; repeat: false
        onTriggered: {
            //console.log("Map initialized");
            //console.log("levelTiles: "+mapProvider.levelTiles)
            //console.log("maxTiles: "+mapProvider.maxTiles)
            flickable.jumpToLL(flickable.defaultLL.x,flickable.defaultLL.y);
            //flickable.flick(-1000,-1000);

        }
    }

    //object tracking
    property point trackXY
    property bool tracking: false
    property double track_dt: 1 //[sec]
    property double track_startTime:0
    property bool trackingJump: false
    /*function startTracking(pointXY) {
        console.log("startTracking");
        //flickable.trackingJump=true;
        flickable.trackXY=Qt.binding(function(){return pointXY;});
        //flickable.trackXY=pointXY;
        flickable.tracking=true;
        //trackTimer.start();
        updateTracking();
    }*/
    function stopTracking() {
        if(!tracking)return;
        console.log("stopTracking");
        trackXY=trackXY;
        tracking=false;
    }
    function updateTracking()
    {
        if(flickable.trackingJump){
            flickable.trackingJump=false;
            jumpToXY(trackXY.x,trackXY.y);
            return false;
        }
        console.log("updateTracking");
        var vel=Qt.point(flickable.center.x-flickable.trackXY.x,flickable.center.y-flickable.trackXY.y);
        var avel=Qt.point(Math.abs(vel.x)*flickable.constSceneXY,Math.abs(vel.y)*flickable.constSceneXY);
        if(avel.x<10 && avel.y<10){
            console.log(avel);
            return false;
        }
        var t=new Date().getTime()/1000;
        var dt=Math.min(1,Math.max(0.1,t-flickable.track_startTime));
        track_dt=track_dt*0.99+dt*0.01;
        flickable.track_startTime=t;
        console.log("track_dt: "+track_dt);
        tracking=true;
        //trackTimer.stop();
        //trackTimer.start();
        var vx=vel.x*flickable.constSceneXY;
        var vy=vel.y*flickable.constSceneXY;
        if(Math.abs(vx)>flickable.width || Math.abs(vy)>flickable.height)jumpToXY(trackXY.x,trackXY.y);
        else {
            flickable.flick(vx,vy);
            console.log("flick: "+vx/flickable.track_dt+","+vy/flickable.track_dt);
        }
        return true;
    }
    Timer {
        id: trackTimer
        interval: Math.min(2000,flickable.track_dt*2000); running: false; repeat: false
        onTriggered: {
            if(flickable.updateTracking()){
                trackTimer.start();
                return;
            }
            console.log("trackTimer timeout");
            flickable.tracking=false;
            //flickable.cancelFlick();
        }
    }
    onTrackXYChanged: {
        console.log("onTrackXYChanged");
        //updateTracking();
    }
    onDragStarted: {
        //console.log("onDragStarted");
        stopTracking();
    }


    //flickable tuning
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    contentWidth: (mapProvider.levelTiles-shiftXY.x)*mapProvider.tileSize
    contentHeight: (mapProvider.levelTiles-shiftXY.y)*mapProvider.tileSize
    boundsBehavior: Flickable.StopAtBounds
    pixelAligned: false //true
    pressDelay: 100
    clip: true

    //exported properties
    property point mouse
    property point mouseLL:  Qt.point(mapProvider.yToLat(mouse.y),mapProvider.xToLon(mouse.x))

    property point center: Qt.point(cX/mapProvider.levelTiles,cY/mapProvider.levelTiles)
    onCenterChanged: mapProvider.center=center;


    //units - [tile]
    property double mX: shiftXY.x + (mousePosX + contentX)/mapProvider.tileSize
    property double mY: shiftXY.y + (mousePosY + contentY)/mapProvider.tileSize
    onMXChanged: mouse.x=(mX/mapProvider.levelTiles);
    onMYChanged: mouse.y=(mY/mapProvider.levelTiles);
    property double cX: shiftXY.x+(contentX+width/2)/contentWidth*(mapProvider.levelTiles-shiftXY.x)
    property double cY: shiftXY.y+(contentY+height/2)/contentHeight*(mapProvider.levelTiles-shiftXY.y)

    //shift adjust
    property point shiftXY: Qt.point(0,0) //[tiles]
    property int shiftStepX: Math.floor(cX/100)*100;
    property int shiftStepY: Math.floor(cY/100)*100;
    property bool shiftReset: false
    function updateShift()
    {
       shiftTimer.start();
    }
    function updateShiftImmediate()
    {
        if(!flickable.optimizeShift)return;
        //console.log("updateShiftImmediate");
        if(flickable.zooming)return;
        var cx=flickable.cX;
        var cy=flickable.cY;
        //var shiftXY_s=flickable.shiftXY;
        //var center_s=flickable.center;
        if(flickable.shiftReset || mapProvider.level<10){
            flickable.shiftReset=false
            flickable.shiftXY=Qt.point(1,1);
            flickable.shiftXY=Qt.point(0,0);
        }else if(flickable.optimizeShift){
            flickable.shiftXY.x=Math.max(0,flickable.shiftStepX-100);
            flickable.shiftXY.y=Math.max(0,flickable.shiftStepY-100);
        }
        flickable.contentX+=(cx-flickable.cX)*mapProvider.tileSize;
        flickable.contentY+=(cy-flickable.cY)*mapProvider.tileSize;
        //flickable.contentX-=(flickable.shiftXY.x-shiftXY_s.x)*mapProvider.tileSize;
        //flickable.contentY-=(flickable.shiftXY.y-shiftXY_s.y)*mapProvider.tileSize;
        //lickable.jumpToXY(center_s.x,center_s.y)
    }
    onMovementEnded: updateShift();
    onShiftXYChanged: {
        console.log("shiftXY: ("+shiftXY.x+","+shiftXY.y+")")
    }
    Timer {
        id: shiftTimer
        interval: 100; running: false; repeat: false
        onTriggered: {
            flickable.updateShiftImmediate();
        }
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
                    flickable.tmpScale=1
                    mapProvider.level+=flickable.zoomingIn?1:-1;
                    flickable.zooming=false;
                    flickable.shiftReset=true;
                    flickable.updateShiftImmediate();
                    if(flickable.tracking){
                        flickable.cancelFlick();
                        flickable.trackingJump=true;
                        flickable.jumpToXY(flickable.cScale.x,flickable.cScale.y);
                    }else{
                        //flickable.jumpToXY(flickable.center.x+flickable.cScale.x-flickable.mouse.x,flickable.center.y+flickable.cScale.y-flickable.mouse.y);
                        //flickable.jumpToXY(flickable.center.x+flickable.cScale.x-flickable.mouse.x,flickable.center.y+flickable.cScale.y-flickable.mouse.y);
                        flickable.contentX=flickable.sceneX(flickable.cScale.x)-flickable.cScaleMS.x;
                        flickable.contentY=flickable.sceneY(flickable.cScale.y)-flickable.cScaleMS.y;
                    }
                    flickable.interactive=true;
                    flickable.updateShiftImmediate();
                }
            }
        }
    }
    transform: Scale {
        origin.x: flickable.sceneX(flickable.cScale.x)-flickable.contentX
        origin.y: flickable.sceneY(flickable.cScale.y)-flickable.contentY
        xScale: flickable.tmpScale;
        yScale: flickable.tmpScale;
    }
    //onTmpScaleChanged: flickable.cancelFlick();
    function zoomIn(){flickable.zoomAdjust(true)}
    function zoomOut(){flickable.zoomAdjust(false)}
    function zoomAdjust(zoomIn)
    {
        flickable.cancelFlick();
        if(flickable.zooming || shiftTimer.running)return;
        //console.log("zoomAdjust");
        //console.log(zoomIn?"zoomIn":"zoomOut");
        flickable.zoomingIn=zoomIn;
        if(zoomIn){
            if(mapProvider.level>=mapProvider.maxLevel)return;
        }else{
            if(mapProvider.level<=minLevel)return;
            //flickable.shiftReset=true;
        }
        flickable.updateShiftImmediate();
        flickable.zooming=true;
        flickable.cScaleMS=Qt.point(flickable.mousePosX,flickable.mousePosY);
        if(flickable.tracking){
            //center aligned scale
            //flickable.cancelFlick();
            cScale=flickable.center;
        }else{
            //mouse aligned scale
            cScale=flickable.mouse;
        }
        scaleAnim.enabled=true;
        flickable.tmpScale*=zoomIn>0?2:0.5
    }

    property int mousePosX: 0 //mouse pos in viewport coordinates
    property int mousePosY: 0
    MouseArea {
         id: mouseArea
         anchors.fill: parent
         enabled: true
         hoverEnabled: true
         propagateComposedEvents: true
         //z: 100
         onPositionChanged: {
             flickable.mousePosX=mouseX-flickable.contentX;//visibleArea.xPosition*flickable.contentWidth;
             flickable.mousePosY=mouseY-flickable.contentY;//visibleArea.yPosition*flickable.contentHeight;
         }
         onClicked: {
             console.log("CLICKED ON UPPER")
             mouse.accepted = false
         }
         onPressed: {
             //console.log("PRESSED ON UPPER")
             flickable.deselect();
             mouse.accepted = false
         }
         onDoubleClicked: {
             console.log("map onDoubleClicked")
             flickable.zoomIn();
         }
         onWheel: {
             //console.log("WHEEL")
             if (wheel.modifiers & Qt.ControlModifier){
                 if (wheel.angleDelta.y > 0) flickable.zoomIn()
                 else flickable.zoomOut()
             }else{
                 //flicking
                 flickable.stopTracking();
                 wheel.accepted=false
             }
         }
    }
    QmlMapTiles {
        id: mapTiles
        x: flickable.constShiftX
        y: flickable.constShiftY
        provider: mapProvider
        visibleArea: Qt.rect(
                         flickable.contentX-flickable.constShiftX,
                         flickable.contentY-flickable.constShiftY,
                         flickable.width,flickable.height
                         )
    }

}
