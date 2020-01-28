import QtQuick 2.11

import APX.Facts 1.0

Fact {
    Fact {
        title: mandala.ctr.title
        icon: "dip-switch"
        Fact { bind: mandala.ctr.str.brake; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.wing.flaps; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.wing.airbrk; flags: Fact.Bool; }
        Fact {
            title: mandala.ctr.light.title
            Fact { bind: mandala.ctr.light.nav; flags: Fact.Bool; }
            Fact { bind: mandala.ctr.light.taxi; flags: Fact.Bool; }
            Fact { bind: mandala.ctr.light.strobe; flags: Fact.Bool; }
            Fact { bind: mandala.ctr.light.beacon; flags: Fact.Bool; }
            Fact { bind: mandala.ctr.light.landing; flags: Fact.Bool; }
        }
    }
    Fact {
        title: mandala.ctr.eng.title
        icon: "engine"
        Fact { bind: mandala.ctr.eng.choke; flags: Fact.Bool; }
        Fact { bind: mandala.cmd.opt.thrcut; flags: Fact.Bool; }
        Fact { bind: mandala.cmd.opt.throvr; flags: Fact.Bool; }
        Fact { enabled: false }
        Fact { bind: mandala.ctr.pwr.ignition; flags: Fact.Bool; }
        Fact { bind: mandala.sns.btn.starter; flags: Fact.Bool; active: mandala.ctr.sw.starter.value; }
    }
    Fact {
        title: mandala.ctr.pwr.title
        icon: "power-standby"
        Fact { bind: mandala.ctr.pwr.payload; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.pwr.xpdr; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.pwr.agl; flags: Fact.Bool; }
        Fact { enabled: false }
        Fact { bind: mandala.ctr.pwr.servo; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.pwr.satcom; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.pwr.rfamp; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.pwr.ice; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.pwr.las; flags: Fact.Bool; }
    }
    Fact {
        title: qsTr("Service")
        icon: "settings"
        Fact {
            title: qsTr("AHRS")
            Fact { title: qsTr("Reset gps home altitude"); descr: "hmsl()"; onTriggered: application.jsexec(descr); }
            Fact { title: qsTr("Reset static pressure"); descr: "zps()"; onTriggered: application.jsexec(descr); }
            Fact { enabled: false }
            Fact { title: qsTr("Reset AHRS filter"); descr: "ahrs()"; onTriggered: application.jsexec(descr); }
            Fact { enabled: false }
            Fact { bind: mandala.cmd.opt.ahrs }
        }
    }
    Fact {
        title: qsTr("Emergency")
        icon: "alert-box"
        Fact { bind: mandala.ctr.ers.launch; flags: Fact.Bool; }
        Fact { bind: mandala.ctr.ers.rel; flags: Fact.Bool; }
    }
}
