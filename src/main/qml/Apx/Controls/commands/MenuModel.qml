/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.11

import APX.Facts 1.0

Fact {
    Fact {
        title: mandala.ctr.title
        icon: "dip-switch"
        Fact { binding: mandala.ctr.str.brake; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.wing.flaps; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.wing.airbrk; flags: Fact.Bool; }
        Fact {
            title: mandala.ctr.light.title
            Fact { binding: mandala.ctr.light.nav; flags: Fact.Bool; }
            Fact { binding: mandala.ctr.light.taxi; flags: Fact.Bool; }
            Fact { binding: mandala.ctr.light.strobe; flags: Fact.Bool; }
            Fact { binding: mandala.ctr.light.beacon; flags: Fact.Bool; }
            Fact { binding: mandala.ctr.light.landing; flags: Fact.Bool; }
        }
    }
    Fact {
        title: mandala.ctr.eng.title
        icon: "engine"
        Fact { binding: mandala.ctr.eng.choke; flags: Fact.Bool; }
        Fact { binding: mandala.cmd.eng.cut; flags: Fact.Bool; }
        Fact { binding: mandala.cmd.eng.ovr; flags: Fact.Bool; }
        Fact { enabled: false }
        Fact { binding: mandala.ctr.pwr.ignition; flags: Fact.Bool; }
        Fact {
            title: mandala.ctr.eng.starter.title;
            descr: mandala.ctr.eng.starter.descr;
            flags: Fact.Bool;
            active: mandala.ctr.eng.starter.value;
            onValueChanged: {
                if(value){
                    mandala.sns.eng.status.value = eng_status_start
                }else{
                    mandala.ctr.eng.starter.value = false;
                }
            }
            property bool v: mandala.sns.eng.status.value === eng_status_start
            onVChanged: value=v
        }
    }
    Fact {
        title: mandala.ctr.pwr.title
        icon: "power-standby"
        Fact { binding: mandala.ctr.pwr.payload; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.pwr.xpdr; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.pwr.agl; flags: Fact.Bool; }
        Fact { enabled: false }
        Fact { binding: mandala.ctr.pwr.servo; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.pwr.satcom; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.pwr.rfamp; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.pwr.ice; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.pwr.las; flags: Fact.Bool; }
    }
    Fact {
        title: qsTr("Service")
        icon: "cog"
        Fact {
            title: qsTr("AHRS")
            Fact { title: qsTr("Reset gps home altitude"); descr: "hmsl()"; onTriggered: application.jsexec(descr); }
            Fact { title: qsTr("Reset static pressure"); descr: "zps()"; onTriggered: application.jsexec(descr); }
            Fact { enabled: false }
            Fact { title: qsTr("Reset AHRS filter"); descr: "ahrs()"; onTriggered: application.jsexec(descr); }
            Fact { enabled: false }
            Fact { binding: mandala.cmd.ahrs.inair; flags: Fact.Bool; }
            Fact { binding: mandala.cmd.ahrs.nogps; flags: Fact.Bool; }
            Fact { binding: mandala.cmd.ahrs.nomag; flags: Fact.Bool; }
            Fact { binding: mandala.cmd.ahrs.hsrc }
        }
    }
    Fact {
        title: qsTr("Emergency")
        icon: "alert-box"
        Fact { binding: mandala.ctr.ers.launch; flags: Fact.Bool; }
        Fact { binding: mandala.ctr.ers.rel; flags: Fact.Bool; }
    }
}
