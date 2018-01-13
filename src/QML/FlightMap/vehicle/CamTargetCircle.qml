import QtQuick 2.5
import QtLocation 5.6
import QtPositioning 5.6
import "./helper.js" as Helper

MapCircle {
    color: "transparent"
    border.color: "#ff0"
    border.width: 20
    smooth: true
    radius: 50
    opacity: 0.8

    //Fact bindings
    property real lat: m.gps_lat.value
    property real lon: m.gps_lon.value
    property real power_payload: m.power_payload.value
    property real cam_roll: m.cam_roll.value
    property real cam_pitch: m.cam_pitch.value
    property real cam_yaw: m.cam_yaw.value
    property real home_hmsl: m.home_hmsl.value
    property real gps_hmsl: m.gps_hmsl.value
    property real yaw: m.yaw.value

    //calculate
    property bool valid: power_payload>0 && camAlt>0 && cam_roll!==0 && cam_pitch!==0 && cam_yaw!==0
    property bool camAlt: gps_hmsl-home_hmsl
    property bool bCamFront: cam_roll !== 0
    property real azimuth: bCamFront?yaw:cam_yaw

    property real distance: 0
    center: QtPositioning.coordinate(lat,lon).atDistanceAndAzimuth(distance,azimuth)

    //visual
    //visible: altitude>5 && gSpeed>1
    //Behavior on distance { enabled: app.settings.smooth.value; NumberAnimation {duration: 5000;} }
}

/*
  double camAlt=mvar->gps_pos[2]-mvar->home_pos[2];
  if((mvar->power&power_payload)&&(camAlt>0)&&(!mvar->cam_theta.isNull())){
    Point camTargetNE;
    if(mvar->cam_theta[0]==0.0){
      //down looking gimal
      camTargetNE=mvar->rotate(Point(camAlt*tan((90.0+mvar->cam_theta[1])*D2R),0),-mvar->cam_theta[2]);
    }else{
      //front gimbal
      camTargetNE[1]=camAlt*tan(-mvar->cam_theta[0]*D2R);
      camTargetNE[0]=mvar->distance(camAlt,camTargetNE[1])*tan((90.0+mvar->cam_theta[1])*D2R);
      camTargetNE=mvar->rotate(camTargetNE,-mvar->theta[2]);
    }
    camTarget->setPos(pos()+QPointF(camTargetNE[1],-camTargetNE[0])*sf);
    camTarget->setVisible(m_current);
    //update track pos for stats
    Point ll=mvar->ne2ll(camTargetNE,Vect(posLL.x(),posLL.y(),mvar->home_pos[2]));
    mvar->cam_tpos=Vect(ll[0],ll[1],mvar->home_pos[2]);
  }else camTarget->setVisible(false);




*/
