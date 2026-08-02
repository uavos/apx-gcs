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
                color: (valueL || valueC || valueR) ? "#ffffff" : "#667788"
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

                        BitRow { label: "0  tilt_align";             valueL: root.bitLL(0); valueC: root.bitL(0); valueR: root.bitRL(0) }
                        BitRow { label: "1  yaw_align";              valueL: root.bitLL(1); valueC: root.bitL(1); valueR: root.bitRL(1) }
                        BitRow { label: "2  gnss_pos";               valueL: root.bitLL(2); valueC: root.bitL(2); valueR: root.bitRL(2) }
                        BitRow { label: "3  opt_flow";               valueL: root.bitLL(3); valueC: root.bitL(3); valueR: root.bitRL(3) }
                        BitRow { label: "4  mag_hdg";                valueL: root.bitLL(4); valueC: root.bitL(4); valueR: root.bitRL(4) }
                        BitRow { label: "5  mag_3D";                 valueL: root.bitLL(5); valueC: root.bitL(5); valueR: root.bitRL(5) }
                        BitRow { label: "6  mag_dec";                valueL: root.bitLL(6); valueC: root.bitL(6); valueR: root.bitRL(6) }
                        BitRow { label: "7  in_air";                 valueL: root.bitLL(7); valueC: root.bitL(7); valueR: root.bitRL(7) }
                        BitRow { label: "8  wind";                   valueL: root.bitLL(8); valueC: root.bitL(8); valueR: root.bitRL(8) }
                        BitRow { label: "9  baro_hgt";               valueL: root.bitLL(9); valueC: root.bitL(9); valueR: root.bitRL(9) }
                        BitRow { label: "10 rng_hgt";                valueL: root.bitLL(10); valueC: root.bitL(10); valueR: root.bitRL(10) }
                        BitRow { label: "11 gps_hgt";                valueL: root.bitLL(11); valueC: root.bitL(11); valueR: root.bitRL(11) }
                        BitRow { label: "12 ev_pos";                 valueL: root.bitLL(12); valueC: root.bitL(12); valueR: root.bitRL(12) }
                        BitRow { label: "13 ev_yaw";                 valueL: root.bitLL(13); valueC: root.bitL(13); valueR: root.bitRL(13) }
                        BitRow { label: "14 ev_hgt";                 valueL: root.bitLL(14); valueC: root.bitL(14); valueR: root.bitRL(14) }
                        BitRow { label: "15 fuse_beta";              valueL: root.bitLL(15); valueC: root.bitL(15); valueR: root.bitRL(15) }
                        BitRow { label: "16 mag_field_disturbed";    valueL: root.bitLL(16); valueC: root.bitL(16); valueR: root.bitRL(16); onColor: "#cc2222" }
                        BitRow { label: "17 fixed_wing";             valueL: root.bitLL(17); valueC: root.bitL(17); valueR: root.bitRL(17) }
                        BitRow { label: "18 mag_fault";              valueL: root.bitLL(18); valueC: root.bitL(18); valueR: root.bitRL(18); onColor: "#cc2222" }
                        BitRow { label: "19 fuse_aspd";              valueL: root.bitLL(19); valueC: root.bitL(19); valueR: root.bitRL(19) }
                        BitRow { label: "20 gnd_effect";             valueL: root.bitLL(20); valueC: root.bitL(20); valueR: root.bitRL(20); onColor: "#ffe600" }
                        BitRow { label: "21 rng_stuck";              valueL: root.bitLL(21); valueC: root.bitL(21); valueR: root.bitRL(21); onColor: "#cc2222" }
                        BitRow { label: "22 gnss_yaw";               valueL: root.bitLL(22); valueC: root.bitL(22); valueR: root.bitRL(22) }
                        BitRow { label: "23 mag_aligned_in_flight";  valueL: root.bitLL(23); valueC: root.bitL(23); valueR: root.bitRL(23) }
                        BitRow { label: "24 ev_vel";                 valueL: root.bitLL(24); valueC: root.bitL(24); valueR: root.bitRL(24) }
                        BitRow { label: "25 synthetic_mag_z";        valueL: root.bitLL(25); valueC: root.bitL(25); valueR: root.bitRL(25) }
                        BitRow { label: "26 vehicle_at_rest";        valueL: root.bitLL(26); valueC: root.bitL(26); valueR: root.bitRL(26) }
                        BitRow { label: "27 gnss_yaw_fault";         valueL: root.bitLL(27); valueC: root.bitL(27); valueR: root.bitRL(27); onColor: "#cc2222" }
                        BitRow { label: "28 rng_fault";              valueL: root.bitLL(28); valueC: root.bitL(28); valueR: root.bitRL(28); onColor: "#cc2222" }
                        BitRow { label: "29 inertial_dead_reckoning"; valueL: root.bitLL(29); valueC: root.bitL(29); valueR: root.bitRL(29); onColor: "#cc2222" }
                        BitRow { label: "30 wind_dead_reckoning";    valueL: root.bitLL(30); valueC: root.bitL(30); valueR: root.bitRL(30); onColor: "#ffe600" }
                        BitRow { label: "31 rng_kin_consistent";     valueL: root.bitLL(31); valueC: root.bitL(31); valueR: root.bitRL(31) }
                    }

                    Column {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 3 * root.sf

                        BitRow { label: "32 fake_pos";               valueL: root.bitLH(32); valueC: root.bitH(32); valueR: root.bitRH(32); onColor: "#ffe600" }
                        BitRow { label: "33 fake_hgt";               valueL: root.bitLH(33); valueC: root.bitH(33); valueR: root.bitRH(33); onColor: "#ffe600" }
                        BitRow { label: "34 gravity_vector";         valueL: root.bitLH(34); valueC: root.bitH(34); valueR: root.bitRH(34) }
                        BitRow { label: "35 mag";                    valueL: root.bitLH(35); valueC: root.bitH(35); valueR: root.bitRH(35) }
                        BitRow { label: "36 ev_yaw_fault";           valueL: root.bitLH(36); valueC: root.bitH(36); valueR: root.bitRH(36); onColor: "#cc2222" }
                        BitRow { label: "37 mag_heading_consistent"; valueL: root.bitLH(37); valueC: root.bitH(37); valueR: root.bitRH(37) }
                        BitRow { label: "38 aux_gpos";               valueL: root.bitLH(38); valueC: root.bitH(38); valueR: root.bitRH(38) }
                        BitRow { label: "39 rng_terrain";            valueL: root.bitLH(39); valueC: root.bitH(39); valueR: root.bitRH(39) }
                        BitRow { label: "40 opt_flow_terrain";       valueL: root.bitLH(40); valueC: root.bitH(40); valueR: root.bitRH(40) }
                        BitRow { label: "41 valid_fake_pos";         valueL: root.bitLH(41); valueC: root.bitH(41); valueR: root.bitRH(41) }
                        BitRow { label: "42 constant_pos";           valueL: root.bitLH(42); valueC: root.bitH(42); valueR: root.bitRH(42) }
                        BitRow { label: "43 baro_fault";             valueL: root.bitLH(43); valueC: root.bitH(43); valueR: root.bitRH(43); onColor: "#cc2222" }
                        BitRow { label: "44 gnss_vel";               valueL: root.bitLH(44); valueC: root.bitH(44); valueR: root.bitRH(44) }
                        BitRow { label: "45 gnss_fault";             valueL: root.bitLH(45); valueC: root.bitH(45); valueR: root.bitRH(45); onColor: "#cc2222" }
                        BitRow { label: "46 yaw_manual";             valueL: root.bitLH(46); valueC: root.bitH(46); valueR: root.bitRH(46) }
                        BitRow { label: "47 gnss_hgt_fault";         valueL: root.bitLH(47); valueC: root.bitH(47); valueR: root.bitRH(47); onColor: "#cc2222" }
                        BitRow { label: "48 in_transition_to_fw";    valueL: root.bitLH(48); valueC: root.bitH(48); valueR: root.bitRH(48) }
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

                        BitRow { label: "0  bad_mag_x";        valueL: root.bitLF(0); valueC: root.bitF(0); valueR: root.bitRF(0);  onColor: "#cc2222" }
                        BitRow { label: "1  bad_mag_y";        valueL: root.bitLF(1); valueC: root.bitF(1); valueR: root.bitRF(1);  onColor: "#cc2222" }
                        BitRow { label: "2  bad_mag_z";        valueL: root.bitLF(2); valueC: root.bitF(2); valueR: root.bitRF(2);  onColor: "#cc2222" }
                        BitRow { label: "3  bad_hdg";          valueL: root.bitLF(3); valueC: root.bitF(3); valueR: root.bitRF(3);  onColor: "#cc2222" }
                        BitRow { label: "4  bad_mag_decl";     valueL: root.bitLF(4); valueC: root.bitF(4); valueR: root.bitRF(4);  onColor: "#cc2222" }
                        BitRow { label: "5  bad_airspeed";     valueL: root.bitLF(5); valueC: root.bitF(5); valueR: root.bitRF(5);  onColor: "#cc2222" }
                        BitRow { label: "6  bad_sideslip";     valueL: root.bitLF(6); valueC: root.bitF(6); valueR: root.bitRF(6);  onColor: "#cc2222" }
                        BitRow { label: "7  bad_optflow_X";    valueL: root.bitLF(7); valueC: root.bitF(7); valueR: root.bitRF(7);  onColor: "#cc2222" }
                        BitRow { label: "8  bad_optflow_Y";    valueL: root.bitLF(8); valueC: root.bitF(8); valueR: root.bitRF(8);  onColor: "#cc2222" }
                        BitRow { label: "9  (unused)";         valueL: root.bitLF(9); valueC: root.bitF(9); valueR: root.bitRF(9);  onColor: "#555" }
                        BitRow { label: "10 bad_acc_vertical"; valueL: root.bitLF(10); valueC: root.bitF(10); valueR: root.bitRF(10); onColor: "#cc2222" }
                        BitRow { label: "11 bad_acc_clipping"; valueL: root.bitLF(11); valueC: root.bitF(11); valueR: root.bitRF(11); onColor: "#cc2222" }
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

                        BitRow { label: "0  gps_checks_passed";          valueL: root.bitLE(0); valueC: root.bitE(0); valueR: root.bitRE(0) }
                        BitRow { label: "1  reset_vel_to_gps";           valueL: root.bitLE(1); valueC: root.bitE(1); valueR: root.bitRE(1);  onColor: "#ffe600" }
                        BitRow { label: "2  reset_vel_to_flow";          valueL: root.bitLE(2); valueC: root.bitE(2); valueR: root.bitRE(2);  onColor: "#ffe600" }
                        BitRow { label: "3  reset_vel_to_vision";        valueL: root.bitLE(3); valueC: root.bitE(3); valueR: root.bitRE(3);  onColor: "#ffe600" }
                        BitRow { label: "4  reset_vel_to_zero";          valueL: root.bitLE(4); valueC: root.bitE(4); valueR: root.bitRE(4);  onColor: "#ffe600" }
                        BitRow { label: "5  reset_pos_to_last_known";    valueL: root.bitLE(5); valueC: root.bitE(5); valueR: root.bitRE(5);  onColor: "#ffe600" }
                        BitRow { label: "6  reset_pos_to_gps";           valueL: root.bitLE(6); valueC: root.bitE(6); valueR: root.bitRE(6);  onColor: "#ffe600" }
                        BitRow { label: "7  reset_pos_to_vision";        valueL: root.bitLE(7); valueC: root.bitE(7); valueR: root.bitRE(7);  onColor: "#ffe600" }
                        BitRow { label: "8  starting_gps_fusion";        valueL: root.bitLE(8); valueC: root.bitE(8); valueR: root.bitRE(8) }
                        BitRow { label: "9  starting_vision_pos_fusion"; valueL: root.bitLE(9); valueC: root.bitE(9); valueR: root.bitRE(9) }
                        BitRow { label: "10 starting_vision_vel_fusion"; valueL: root.bitLE(10); valueC: root.bitE(10); valueR: root.bitRE(10) }
                        BitRow { label: "11 starting_vision_yaw_fusion"; valueL: root.bitLE(11); valueC: root.bitE(11); valueR: root.bitRE(11) }
                        BitRow { label: "12 yaw_aligned_to_imu_gps";    valueL: root.bitLE(12); valueC: root.bitE(12); valueR: root.bitRE(12) }
                        BitRow { label: "13 reset_hgt_to_baro";         valueL: root.bitLE(13); valueC: root.bitE(13); valueR: root.bitRE(13); onColor: "#ffe600" }
                        BitRow { label: "14 reset_hgt_to_gps";          valueL: root.bitLE(14); valueC: root.bitE(14); valueR: root.bitRE(14); onColor: "#ffe600" }
                        BitRow { label: "15 reset_hgt_to_rng";          valueL: root.bitLE(15); valueC: root.bitE(15); valueR: root.bitRE(15); onColor: "#ffe600" }
                        BitRow { label: "16 reset_hgt_to_ev";           valueL: root.bitLE(16); valueC: root.bitE(16); valueR: root.bitRE(16); onColor: "#ffe600" }
                        BitRow { label: "17 reset_pos_to_ext_obs";      valueL: root.bitLE(17); valueC: root.bitE(17); valueR: root.bitRE(17); onColor: "#ffe600" }
                        BitRow { label: "18 reset_wind_to_ext_obs";     valueL: root.bitLE(18); valueC: root.bitE(18); valueR: root.bitRE(18); onColor: "#ffe600" }
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
