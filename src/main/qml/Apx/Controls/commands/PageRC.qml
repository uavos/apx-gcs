import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQml.Models 2.12

Page {
    ListView {
        id: listView
        width: parent.width
        height: parent.height
        spacing: 2
        model: ObjectModel {
            CtrSlider { title: "R"; fact: mandala.cmd.rc.roll; width: listView.width }
            CtrSlider { title: "P"; fact: mandala.cmd.rc.pitch; width: listView.width }
            CtrSlider { title: "T"; fact: mandala.cmd.rc.throttle; from: 0; width: listView.width }
            CtrSlider { title: "Y"; fact: mandala.cmd.rc.yaw; width: listView.width }
            CtrSlider { title: "FLP"; fact: mandala.ctr.wing.flaps; from: 0; stepSize: 0.1; width: listView.width }
            CtrSlider { title: "ABR"; fact: mandala.ctr.wing.airbrk; from: 0; stepSize: 0.1; width: listView.width }
            CtrSlider { title: "BRK"; fact: mandala.ctr.str.brake; from: 0; stepSize: 0.1; width: listView.width }
        }
    }
}
