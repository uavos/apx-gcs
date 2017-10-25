import QtQuick 2.2
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import "."
import com.uavos.map 1.0


Item {
    id: objectItem

    property var mItem
    property string text: model.modelData.name
    property alias textColor: textItem.textColor
    property alias textSize: textItem.textSize
    property alias color: textItem.color

    property bool interactive: true

    property bool hover: mouseArea.containsMouse
    property bool dragging: mouseArea.drag.active
    property bool selected: false

    property real hoverScaleFactor: 1.5

    property var parentObject

    signal objectMoved(double mlat,double mlon)

    Behavior on opacity { PropertyAnimation {duration: map.animation_duration*2; } }

    function select()
    {
        if(!selected){
            map.deselect();
            selected=true;
        }
        //map.jumpToLL(lat,lon);
    }
    function deselect()
    {
        selected=false;
    }

    /*Connections {
        target: map
        onDeselect: objectItem.selected=false;
    }*/

    property bool menuVisible: false

    onSelectedChanged: {
        //menuVisible=false;
        //map.flickToXYFinished.disconnect(flickToXYFinished)
        //menu.hide();
        if(selected){
            map.deselect.connect(deselect);
            if(parentObject){
                parentObject.selected=true;
            }
        }else{
            map.deselect.disconnect(deselect);
            menuVisible=false;
        }
    }

    function flickToObject()
    {
        map.flickToXY(gx,gy);
    }

    //object fields edit menu
    function showMenu()
    {
        //map.flickToXYFinished.connect(flickToXYFinished)
        flickToObject();
        menuVisible=true;
    }
    /*function flickToXYFinished()
    {
        map.flickToXYFinished.disconnect(flickToXYFinished)
        menuVisible=true;
        //menu.show();
        //console.log("menu show");
    }*/
    property var menuObject
    onMenuVisibleChanged: {
        //map.flickToXYFinished.disconnect(flickToXYFinished)
        if(menuObject){
            console.log("menu hide");
            menuObject.hide();
            menuObject.destroy();
            menuObject=0;
        }
        if(menuVisible){
            console.log("menu show");
            menuObject=menuComponent.createObject(this);
            menuObject.show();
        }
    }
    Component {
        id: menuComponent
        MapMenu {
            id: menu
            fields: mItem?mItem.items:0
            title: modelData.caption+" "+modelData.name
            info: modelData.value
        }
    }


    //object geometry
    width: textItem.width*mapProvider.itemScaleFactor*hoverScaleFactor
    height: textItem.height*mapProvider.itemScaleFactor*hoverScaleFactor

    z: (dragging||selected||hover)?100:0

    //internal use
    onMItemChanged: {
        if(mItem){
            f_lat=mItem.child("latitude")
            f_lon=mItem.child("longitude")
        }
    }
    onDraggingChanged: if(dragging)select();
    property var f_lat
    property var f_lon
    property double lat: 0
    property double lon: 0

    property double glat: f_lat?m.f_lat.value:lat
    property double glon: f_lon?m.f_lon.value:lon
    //onLatChanged: updateGlobalPosition();
    //onLonChanged: updateGlobalPosition();


    //global positioning
    function snapFromModel(ilat,ilon)
    {
        if(mItem)return;
        glat=ilat
        glon=ilon
        glat=Qt.binding(function() { return lat; })
        glon=Qt.binding(function() { return lon; })

        //gx=mapProvider.lonToX(lon);
        //gy=mapProvider.latToY(lat);
    }
    property double gx: mapProvider.lonToX(glon)
    property double gy: mapProvider.latToY(glat)
    property double px: map.mapToSceneX(gx)
    property double py: map.mapToSceneY(gy)
    x: px-width/2
    y: py-height/2

    Rectangle {
        id: frame
        visible: selected
        anchors.centerIn: textItem
        width: textItem.width*textItem.scale+10*mapProvider.mapScaleFactor/map.tmpScale
        height: textItem.height*textItem.scale+10*mapProvider.mapScaleFactor/map.tmpScale
        //anchors.margins: -5*textItem.scale
        //scale: textItem.scale
        antialiasing: true
        smooth: true
        border.width: 1*mapProvider.mapScaleFactor/map.tmpScale
        border.color: "#FFFFFF"
        radius: 5*mapProvider.mapScaleFactor/map.tmpScale
        color: "#50FFFFFF"
        opacity: 0.5
    }

    property double hoverScale: ((dragging||hover)?hoverScaleFactor:1)
    Behavior on hoverScale { PropertyAnimation {duration: map.animation_duration; } }

    MapText {
        id: textItem
        anchors.centerIn: parent
        scale: hoverScale*mapProvider.itemScaleFactor/map.tmpScale
        textSize: 14
        opacity: dragging?0.6:1
        text: objectItem.text
    }



    MouseArea {
        id: mouseArea
        anchors.fill: parent
        //anchors.centerIn: parent
        //width: parent.width//*hoverScaleFactor
        //height: parent.height//*hoverScaleFactor
        enabled: interactive
        hoverEnabled: enabled
        visible: enabled
        propagateComposedEvents: true
        cursorShape: Qt.PointingHandCursor

        drag.target: objectItem
        drag.axis: Drag.XAndYAxis
        onClicked: {
            //console.log("click object");
            if(selected){
                //flickToObject();
                showMenu();
            }
            select();
        }
        onPositionChanged: {
            if(drag.active){
                var x=objectItem.x+width/2;
                var y=objectItem.y+height/2;
                var vlat=mapProvider.yToLat(map.sceneToGlobalY(y));
                var vlon=mapProvider.xToLon(map.sceneToGlobalX(x));
                if(f_lat)m.f_lat.value=vlat;
                if(f_lon)m.f_lon.value=vlon;
                objectMoved(vlat,vlon);

                //flick when dragging to edges
                var wx=map.sceneToWindowX(x);
                var wy=map.sceneToWindowY(y);
                var ws=mapProvider.tileSize;
                if(wx<0.1)wx=ws;
                else if(wx>0.9)wx=-ws;
                else wx=0;
                if(wy<0.1)wy=ws;
                else if(wy>0.9)wy=-ws;
                else wy=0;
                if(!(wx===0 && wy===0))map.flick(wx,wy);
            }
        }
    }
}
