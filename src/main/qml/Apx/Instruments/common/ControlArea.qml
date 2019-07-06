import QtQuick 2.2

MouseArea {
    id: mouse_area
    property variant mvarX  //roll
    property variant mvarY  //pitch
    property variant mvar   //throttle,airspeed,altitude
    property double min: 0  //for mvar only
    property double max: 1
    property double speed: 1
    property bool fixedPoint: false

    property double span: Math.abs(max-min)
    property double step: span/100
    property double stepDrag: span/2*speed
    property double stepWheel: step*0.001*speed
    property double stepLimit: span/10
    property bool doWheel: true

    property double ptr_size: apx.limit(height*0.1,25,100)
    //anchors.fill: parent
    hoverEnabled: true
    propagateComposedEvents: true
    property bool active: mouse_area.drag.active
    property double out_xv: (mouse_ptr.x+mouse_ptr.width/2)/width*2-1
    property double out_yv: (mouse_ptr.y+mouse_ptr.height/2)/height*2-1
    property double out_x: mouse_ptr.active?out_xv:0
    property double out_y: mouse_ptr.active?out_yv:0

    property double mvar_s: 0
    property double pos_s: 0

    drag.target: mouse_ptr
    drag.axis: Drag.XAndYAxis
    drag.minimumX: -mouse_ptr.width/2
    drag.maximumX: width - mouse_ptr.width/2
    drag.minimumY: -mouse_ptr.height/2
    drag.maximumY: height - mouse_ptr.height/2
    onPressed: {
        mouse_ptr.visible = true;
        mouse_ptr.x = mouse.x-mouse_ptr.width/2
        mouse_ptr.y = mouse.y-mouse_ptr.height/2
        if(mvar)mvar_s=mvar.value;
        pos_s=out_yv
    }
    onReleased: {
        mouse_ptr.visible = false;
        if(mvarX)mvarX.setValue(0);
        if(mvarY)mvarY.setValue(0);
    }
    onPositionChanged: {
        if(drag.active){
            mouse_rect.visible=true;
            if(mvarX)mvarX.setValue(out_x);
            if(mvarY)mvarY.setValue(out_y);
            if(mvar){
                mvar_set(adj(mvar_s,-(out_y-pos_s)*span/2*stepDrag));
                pos_s=out_yv;
            }
        }
    }
    onEntered: mouse_rect.visible=true;

    //mvar op (throttle like)
    function adj(v,vstep) {
        vstep=apx.limit(vstep,-stepLimit,stepLimit);
        mvar_s=apx.limit(v+vstep,min,max);
        return mvar_s;
    }
    function mvar_set(v) {
        if(fixedPoint)v=parseInt(v);
        if(mvar.value!==v) mvar.setValue(v);
    }
    onClicked: {
        if(mvar){
            var sv=out_yv<0?step:-step
            mvar_set(adj(mvar.value,sv))
        }
    }
    onWheel: {
        if(mvar && doWheel){
            var sv=wheel.angleDelta.y*wheel.angleDelta.y*stepWheel*(wheel.angleDelta.y>0?1:-1)
            adj(mvar_timer.running?mvar_s:mvar.value,sv)
            mvar_timer.start()
        }
    }
    Timer {
        id: mvar_timer
        interval: 100
        repeat: false
        running: false
        onTriggered: mvar_set(mvar_s);
    }

    Rectangle {
        id: mouse_ptr
        z: 1000
        property bool active: mouse_area.drag.active
        width: ptr_size
        height: width
        color: "red"
        border.color: active?"#F88":"yellow"
        border.width: active?10:2
        radius: width*0.5
        visible: false
    }
    Rectangle {
        id: mouse_rect
        anchors.fill: parent
        z: 1000
        color: "transparent"
        border.width: 1
        border.color: "#8010f010"
        visible: false
    }
    Timer {
        interval: 1000
        repeat: true
        running: mouse_rect.visible
        onTriggered: mouse_rect.visible=mouse_area.drag.active
    }

}

