import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQml.Models 2.12

Page {
    ListView {
        id: listView
        width: parent.width
        height: parent.height
        spacing: 0
        model: ObjectModel {
            CtrSlider { title: "AIL"; fact: m.rc_roll; width: listView.width }
            CtrSlider { title: "ELV"; fact: m.rc_pitch; width: listView.width }
            CtrSlider { title: "THR"; fact: m.rc_throttle; from: 0; width: listView.width }
            CtrSlider { title: "YAW"; fact: m.rc_yaw; width: listView.width }
            CtrSlider { title: "FLP"; fact: m.ctr_flaps; from: 0; stepSize: 0.1; width: listView.width }
            CtrSlider { title: "BRK"; fact: m.ctr_brake; from: 0; stepSize: 0.1; width: listView.width }
            CtrSlider { title: "ABR"; fact: m.ctr_airbrk; from: 0; stepSize: 0.1; width: listView.width }
        }
    }
}
