import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    color: "#0B1220"

    // UI scale factor
    readonly property real sf: application.scale

    // Mode: false = std (CENTER only), true = ext (LEFT, CENTER, RIGHT)
    property bool extMode: {
        var v = application.prefs.loadValue("extMode", "EKFPlugin", false)
        return v === true || v === "true"
    }
    onExtModeChanged: application.prefs.saveValue("extMode", extMode, "EKFPlugin")

    // HOLD state
    property bool holdActive: false
    property bool holdAlarm: false

    // Alert sound (repeats every 3s until HOLD is dismissed)
    Timer {
        id: alertTimer
        property string sound: ""
        interval: 3000
        repeat: true
        running: root.holdAlarm
        onTriggered: application.sound(sound)
        onRunningChanged: if (!running) sound = ""
    }
    function alertRepeat(snd) {
        holdAlarm = true
        alertTimer.sound = snd
        application.sound(snd)
    }

    // Sound alarm on status change when HOLD is active
    onEkf_left_statuslChanged:  if (holdActive && extMode) alertRepeat("warning")
    onEkf_left_statushChanged:  if (holdActive && extMode) alertRepeat("warning")
    onEkf_left_faultChanged:    if (holdActive && extMode) alertRepeat("error")
    onEkf_left_eventChanged:    if (holdActive && extMode) alertRepeat("warning")
    onEkf_statuslChanged:       if (holdActive) alertRepeat("warning")
    onEkf_statushChanged:       if (holdActive) alertRepeat("warning")
    onEkf_faultChanged:         if (holdActive) alertRepeat("error")
    onEkf_eventChanged:         if (holdActive) alertRepeat("warning")
    onEkf_right_statuslChanged: if (holdActive && extMode) alertRepeat("warning")
    onEkf_right_statushChanged: if (holdActive && extMode) alertRepeat("warning")
    onEkf_right_faultChanged:   if (holdActive && extMode) alertRepeat("error")
    onEkf_right_eventChanged:   if (holdActive && extMode) alertRepeat("warning")

    // LEFT
    readonly property real ekf_left_statusl: mandala.est.usrx.x1.value
    readonly property real ekf_left_statush: mandala.est.usrx.x2.value
    readonly property real ekf_left_fault:   mandala.est.usrx.x3.value
    readonly property real ekf_left_event:   mandala.est.usrx.x4.value

    // CENTER
    readonly property real ekf_statusl: mandala.est.ins.statusl.value
    readonly property real ekf_statush: mandala.est.ins.statush.value
    readonly property real ekf_fault:   mandala.est.ins.fault.value
    readonly property real ekf_event:   mandala.est.ins.event.value

    // RIGHT
    readonly property real ekf_right_statusl: mandala.est.usrx.x5.value
    readonly property real ekf_right_statush: mandala.est.usrx.x6.value
    readonly property real ekf_right_fault:   mandala.est.usrx.x7.value
    readonly property real ekf_right_event:   mandala.est.usrx.x8.value

    // LEFT bit helpers
    function bitLL(n) { return !!((Math.round(ekf_left_statusl) >>> 0) & (1 << n)) }
    function bitLH(n) { return !!((Math.round(ekf_left_statush) >>> 0) & (1 << (n - 32))) }
    function bitLF(n) { return !!((Math.round(ekf_left_fault) >>> 0) & (1 << n)) }
    function bitLE(n) { return !!((Math.round(ekf_left_event) >>> 0) & (1 << n)) }

    // CENTER bit helpers
    function bitL(n) { return !!((Math.round(ekf_statusl) >>> 0) & (1 << n)) }
    function bitH(n) { return !!((Math.round(ekf_statush) >>> 0) & (1 << (n - 32))) }
    function bitF(n) { return !!((Math.round(ekf_fault) >>> 0) & (1 << n)) }
    function bitE(n) { return !!((Math.round(ekf_event) >>> 0) & (1 << n)) }

    // RIGHT bit helpers
    function bitRL(n) { return !!((Math.round(ekf_right_statusl) >>> 0) & (1 << n)) }
    function bitRH(n) { return !!((Math.round(ekf_right_statush) >>> 0) & (1 << (n - 32))) }
    function bitRF(n) { return !!((Math.round(ekf_right_fault) >>> 0) & (1 << n)) }
    function bitRE(n) { return !!((Math.round(ekf_right_event) >>> 0) & (1 << n)) }

    component BitRow: Rectangle {
        property string label: ""
        property bool   valueL: false
        property bool   valueC: false
        property bool   valueR: false
        property color  onColor: "#4ade80"
        property color  offColor: "#333"
        property bool   lostL: false
        property bool   lostC: false
        property bool   lostR: false
        property bool   gainedL: false
        property bool   gainedC: false
        property bool   gainedR: false

        color: "transparent"
        radius: 2
        implicitWidth: rowContent.implicitWidth
        implicitHeight: rowContent.implicitHeight

        onValueLChanged: if (root.holdActive) { if (valueL) gainedL = true; else lostL = true }
        onValueCChanged: if (root.holdActive) { if (valueC) gainedC = true; else lostC = true }
        onValueRChanged: if (root.holdActive) { if (valueR) gainedR = true; else lostR = true }

        Connections {
            target: root
            function onHoldActiveChanged() {
                if (!root.holdActive) { lostL = false; lostC = false; lostR = false; gainedL = false; gainedC = false; gainedR = false }
            }
            function onHoldAlarmChanged() {
                if (!root.holdAlarm) { lostL = false; lostC = false; lostR = false; gainedL = false; gainedC = false; gainedR = false }
            }
        }

        Row {
            id: rowContent
            spacing: 3 * root.sf

            Rectangle {
                width: 12 * root.sf; height: 12 * root.sf; radius: 2
                visible: root.extMode
                anchors.verticalCenter: parent.verticalCenter
                color: lostL ? "#111" : gainedL ? "#ffff44" : valueL ? onColor : offColor
                border.color: (lostL || gainedL) ? "#ffff44" : "#555"; border.width: 1
            }
            Rectangle {
                width: 12 * root.sf; height: 12 * root.sf; radius: 2
                anchors.verticalCenter: parent.verticalCenter
                color: lostC ? "#111" : gainedC ? "#ffff44" : valueC ? onColor : offColor
                border.color: (lostC || gainedC) ? "#ffff44" : "#555"; border.width: 1
            }
            Rectangle {
                width: 12 * root.sf; height: 12 * root.sf; radius: 2
                visible: root.extMode
                anchors.verticalCenter: parent.verticalCenter
                color: lostR ? "#111" : gainedR ? "#ffff44" : valueR ? onColor : offColor
                border.color: (lostR || gainedR) ? "#ffff44" : "#555"; border.width: 1
            }
            Text {
                text: label
                color: (root.extMode ? (valueL || valueC || valueR) : valueC) ? "#ffffff" : "#667788"                
                font.pixelSize: 13 * root.sf
                anchors.verticalCenter: parent.verticalCenter
                leftPadding: 4 * root.sf
            }
        }
    }

    component StatusPanel: Rectangle {
        property string title: ""
        default property alias content: colContent.children
        radius: 8 * root.sf
        color: "#121b25"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8 * root.sf
            spacing: 4 * root.sf

            Text {
                text: parent.parent.title
                color: "#b6b6b6"
                font.pixelSize: 15 * root.sf
                font.bold: true
            }
            Row {
                visible: root.extMode
                spacing: 8 * root.sf
                Text { text: "L"; color: "#556677"; font.pixelSize: 12 * root.sf }
                Text { text: "C"; color: "#556677"; font.pixelSize: 12 * root.sf }
                Text { text: "R"; color: "#556677"; font.pixelSize: 12 * root.sf }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#b6b6b6" }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Column { id: colContent; width: parent.width; spacing: 3 * root.sf }
            }
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 8 * root.sf
        spacing: 8 * root.sf

        RowLayout {
            Layout.fillWidth: true
            spacing: 12 * root.sf

            // Status hex columns
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4 * root.sf

                RowLayout {
                    visible: root.extMode
                    Layout.fillWidth: true; spacing: 8 * root.sf
                    Text { text: "L"; color: "#556677"; font.pixelSize: 11 * root.sf; font.bold: true }
                    Text {
                        property string hexL: ("00000000" + (Math.round(root.ekf_left_statusl)>>>0).toString(16).toUpperCase()).slice(-8)
                        property string hexH: ("00000000" + (Math.round(root.ekf_left_statush)>>>0).toString(16).toUpperCase()).slice(-8)
                        text: "STATUS: 0x" + hexH + hexL; color: "#b6b6b6"; font.pixelSize: 11 * root.sf
                    }
                    Text { text: "FAULT: 0x" + ("00000000"+(Math.round(root.ekf_left_fault)>>>0).toString(16).toUpperCase()).slice(-8); color: "#b6b6b6"; font.pixelSize: 11 * root.sf }
                    Text { text: "EVENT: 0x" + ("00000000"+(Math.round(root.ekf_left_event)>>>0).toString(16).toUpperCase()).slice(-8); color: "#b6b6b6"; font.pixelSize: 11 * root.sf }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8 * root.sf
                    Text { text: "C"; color: "#556677"; font.pixelSize: 11 * root.sf; font.bold: true }
                    Text {
                        property string hexL: ("00000000" + (Math.round(root.ekf_statusl)>>>0).toString(16).toUpperCase()).slice(-8)
                        property string hexH: ("00000000" + (Math.round(root.ekf_statush)>>>0).toString(16).toUpperCase()).slice(-8)
                        text: "STATUS: 0x" + hexH + hexL; color: "#b6b6b6"; font.pixelSize: 11 * root.sf
                    }
                    Text { text: "FAULT: 0x" + ("00000000"+(Math.round(root.ekf_fault)>>>0).toString(16).toUpperCase()).slice(-8); color: "#b6b6b6"; font.pixelSize: 11 * root.sf }
                    Text { text: "EVENT: 0x" + ("00000000"+(Math.round(root.ekf_event)>>>0).toString(16).toUpperCase()).slice(-8); color: "#b6b6b6"; font.pixelSize: 11 * root.sf }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    visible: root.extMode
                    Layout.fillWidth: true; spacing: 8 * root.sf
                    Text { text: "R"; color: "#556677"; font.pixelSize: 11 * root.sf; font.bold: true }
                    Text {
                        property string hexL: ("00000000" + (Math.round(root.ekf_right_statusl)>>>0).toString(16).toUpperCase()).slice(-8)
                        property string hexH: ("00000000" + (Math.round(root.ekf_right_statush)>>>0).toString(16).toUpperCase()).slice(-8)
                        text: "STATUS: 0x" + hexH + hexL; color: "#b6b6b6"; font.pixelSize: 11 * root.sf
                    }
                    Text { text: "FAULT: 0x" + ("00000000"+(Math.round(root.ekf_right_fault)>>>0).toString(16).toUpperCase()).slice(-8); color: "#b6b6b6"; font.pixelSize: 11 * root.sf }
                    Text { text: "EVENT: 0x" + ("00000000"+(Math.round(root.ekf_right_event)>>>0).toString(16).toUpperCase()).slice(-8); color: "#b6b6b6"; font.pixelSize: 11 * root.sf }
                    Item { Layout.fillWidth: true }
                }
            }
        }

        // HOLD button - full width
        Rectangle {
            id: holdBtn
            Layout.fillWidth: true
            Layout.preferredHeight: 24 * root.sf
            radius: 6 * root.sf
            color: root.holdAlarm ? "#5a1a1a" : root.holdActive ? "#2a5a2a" : "#1a1a2a"
            border.color: root.holdAlarm ? "#cc2222" : root.holdActive ? "#44cc44" : "#555"
            border.width: 1

            SequentialAnimation on opacity {
                running: root.holdAlarm
                loops: Animation.Infinite
                NumberAnimation { from: 1; to: 0.3; duration: 400 }
                NumberAnimation { from: 0.3; to: 1; duration: 400 }
                onRunningChanged: if (!running) holdBtn.opacity = 1
            }

            Text {
                anchors.centerIn: parent
                text: root.holdAlarm ? "ALARM" : root.holdActive ? "HOLD ACTIVE" : "HOLD"
                color: root.holdAlarm ? "#ff4444" : root.holdActive ? "#4ade80" : "#8aa0b3"
                font.pixelSize: 12 * root.sf
                font.bold: true
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (root.holdAlarm) {
                        root.holdAlarm = false
                    } else {
                        root.holdActive = !root.holdActive
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8 * root.sf

            StatusPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 7
                Layout.fillHeight: true
                title: "FILTER CONTROL STATUS"

                RowLayout {
                    width: parent.width; spacing: 10 * root.sf

                    Column {
                        Layout.minimumWidth: 80
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 3 * root.sf

                        Repeater {
                            model: filterControlStatusLoModel
                            delegate: BitRow {
                                label: model.label
                                onColor: model.onColor.length > 0 ? model.onColor : "#4ade80"
                                valueL: root.bitLL(model.bit)
                                valueC: root.bitL(model.bit)
                                valueR: root.bitRL(model.bit)
                            }
                        }
                    }

                    Column {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 3 * root.sf

                        Repeater {
                            model: filterControlStatusHiModel
                            delegate: BitRow {
                                label: model.label
                                onColor: model.onColor.length > 0 ? model.onColor : "#4ade80"
                                valueL: root.bitLH(model.bit)
                                valueC: root.bitH(model.bit)
                                valueR: root.bitRH(model.bit)
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 4
                Layout.fillHeight: true
                spacing: 8 * root.sf

                StatusPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 2
                    title: "FAULT STATUS"

                    Column {
                        Layout.minimumWidth: 120
                        width: parent.width
                        spacing: 3 * root.sf

                        Repeater {
                            model: faultStatusModel
                            delegate: BitRow {
                                label: model.label
                                onColor: model.onColor.length > 0 ? model.onColor : "#4ade80"
                                valueL: root.bitLF(model.bit)
                                valueC: root.bitF(model.bit)
                                valueR: root.bitRF(model.bit)
                            }
                        }
                    }
                }

                StatusPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 3
                    title: "EVENT STATUS"

                    Column {
                        width: parent.width
                        spacing: 3 * root.sf

                        Repeater {
                            model: eventStatusModel
                            delegate: BitRow {
                                label: model.label
                                onColor: model.onColor.length > 0 ? model.onColor : "#4ade80"
                                valueL: root.bitLE(model.bit)
                                valueC: root.bitE(model.bit)
                                valueR: root.bitRE(model.bit)
                            }
                        }
                    }
                }

                // STD/EXT mode switch
                Row {
                    Layout.alignment: Qt.AlignRight
                    spacing: -12
                    Text {
                        text: "STD"
                        color: root.extMode ? "#556677" : "#8aa0b3"
                        font.pixelSize: 10 * root.sf
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Switch {
                        scale: 0.5
                        checked: root.extMode
                        onCheckedChanged: root.extMode = checked
                    }
                    Text {
                        text: "EXT"
                        color: root.extMode ? "#66aaff" : "#556677"
                        font.pixelSize: 10 * root.sf
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
