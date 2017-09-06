import QtQuick 2.2
import "."

MouseArea {
    id: mouse_area
    anchors.fill: parent
    property double out_x: mouse_ptr.active?(mouse_ptr.x+mouse_ptr.width/2)/parent.width*2-1:0
    property double out_y: mouse_ptr.active?(mouse_ptr.y+mouse_ptr.height/2)/parent.height*2-1:0
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
    }
    onReleased: {
        mouse_ptr.visible = false;
        rc_roll.setValue(0);
        rc_pitch.setValue(0);
    }
    onPositionChanged: {
        if(drag.active){
            rc_roll.setValue(out_x);
            rc_pitch.setValue(out_y);
        }
    }
    Rectangle {
        id: mouse_ptr
        property bool active: mouse_area.drag.active
        width: 25
        height: width
        color: "red"
        border.color: active?"#F88":"yellow"
        border.width: active?10:2
        radius: width*0.5
        visible: false
    }
}
