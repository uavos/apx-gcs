//NumbersPlugin: rpm::0:value>5700:value>=5900 || error_rpm.value,OP::1:(value>0 && value<2) || value>=6:value>=7,OT::0:(value>0 && value<50) || value>=110:value>=130,ET::0:(value>0 && value<60) || value>=100:value>=120,EGT::0:value>0 && diff>=500:value>=980,fuel::1:value>0 && value<20:value>0 && value<10,cas2tas::1:value>0 && (value<1 || value>1.6):value>1.8,userb_1:TWARN:0:value>0,userb_2:TCAUT:0::value>0,userb_5:IGN1:0:value==0,userb_6:IGN2:0:value==0,Vs:BAT:1:value>0 && value<13.1:value>0 && value<12.9
import QtQuick 2.2
import "../common"
Item {
  Column {
    anchors.fill: parent
    spacing: -3
    property double txtHeight: apx.limit(-spacing+height/12,10,width/4)
    Number {
      height: parent.txtHeight
      mfield: m.rpm
      precision: 0
      warning: value>5700
      alarm: value>=5900 || m.error_rpm.value
    }
    Number {
      height: parent.txtHeight
      mfield: m.OP
      precision: 1
      warning: (value>0 && value<2) || value>=6
      alarm: value>=7
    }
    Number {
      height: parent.txtHeight
      mfield: m.OT
      precision: 0
      warning: (value>0 && value<50) || value>=110
      alarm: value>=130
    }
    Number {
      height: parent.txtHeight
      mfield: m.ET
      precision: 0
      warning: (value>0 && value<60) || value>=100
      alarm: value>=120
    }
    Number {
      height: parent.txtHeight
      mfield: m.EGT
      precision: 0
      warning: value>0 && diff>=500
      alarm: value>=980
    }
    Number {
      height: parent.txtHeight
      mfield: m.fuel
      precision: 1
      warning: value>0 && value<20
      alarm: value>0 && value<10
    }
    Number {
      height: parent.txtHeight
      mfield: m.cas2tas
      precision: 1
      warning: value>0 && (value<1 || value>1.6)
      alarm: value>1.8
    }
    Number {
      height: parent.txtHeight
      mfield: m.userb_1
      label: "TWARN"
      precision: 0
      warning: value>0
    }
    Number {
      height: parent.txtHeight
      mfield: m.userb_2
      label: "TCAUT"
      precision: 0
      alarm: value>0
    }
    Number {
      height: parent.txtHeight
      mfield: m.userb_5
      label: "IGN1"
      precision: 0
      warning: value==0
    }
    Number {
      height: parent.txtHeight
      mfield: m.userb_6
      label: "IGN2"
      precision: 0
      warning: value==0
    }
    Number {
      height: parent.txtHeight
      mfield: m.Vs
      label: "BAT"
      precision: 1
      warning: value>0 && value<13.0
      alarm: value>0 && value<12.4
    }
  }
}
