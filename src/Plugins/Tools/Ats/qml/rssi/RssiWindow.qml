import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: rssiPage

    // f_rssi fact of the Ats plugin
    property var fact

    // Ats plugin root fact (Q_INVOKABLE methods live here)
    readonly property var ats: fact ? fact.parentFact : null

    title: qsTr("RSSI")
    width: 1180
    height: 700
    minimumWidth: 860
    minimumHeight: 700
    color: "#10141f"
    visible: true
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint | Qt.WindowMinMaxButtonsHint

    onClosing: {
        if (fact)
            fact.value = false
    }

    // Bias facts of the ATS plugin (actual values shown on open)
    readonly property var biasYawFact: ats ? ats.child("bias").child("yaw") : null
    readonly property var biasPitchFact: ats ? ats.child("bias").child("pitch") : null

    // Heatmap data: key "az_el" (binned) -> rssi value
    property var rssiData: ({})
    readonly property int binSize: 1 // deg per cell

    // Axis ranges
    readonly property real azMin: 0
    readonly property real azMax: 360
    readonly property real elMin: 0
    readonly property real elMax: 90

    property real currentYaw: 0
    property real currentPitch: 0
    property real currentRssi: 0
    property real losDistance: 0

    // Commanded antenna angles (manual control)
    property real cmdYaw: 0
    property real cmdPitch: 0

    // ATS mode (polled from GCS mandala)
    property string modeText: ""

    property real maxRssi: 0
    property real maxRssiAz: 0
    property real maxRssiEl: 0

    property bool dataCollectionPaused: false

    // Deviation: antenna pointing vs max signal point
    readonly property real deltaAz: {
        var d = maxRssiAz - currentYaw
        if (d > 180) d -= 360
        if (d < -180) d += 360
        return d
    }
    readonly property real deltaEl: maxRssiEl - currentPitch

    // Scan sector (scan logic runs on the modem side)
    readonly property real scanAzRangeMax: 60
    readonly property real scanElRangeMax: 30
    property real scanAzRange: scanAzRangeMax
    property real scanElRange: scanElRangeMax
    readonly property bool scanning: modeText === "search"

    function colorFor(v) {
        // v: 0..100, red (10%) - yellow (50%) - green (100%)
        var t = Math.max(0, Math.min(1, v / 100))
        var stops = [
            [0.00, 0xd2, 0x28, 0x1e],
            [0.10, 0xd2, 0x28, 0x1e],
            [0.50, 0xff, 0xc8, 0x14],
            [1.00, 0x14, 0xb4, 0x28]
        ]
        for (var i = 1; i < stops.length; i++) {
            if (t <= stops[i][0]) {
                var a = stops[i - 1], b = stops[i]
                var f = (t - a[0]) / (b[0] - a[0])
                var r = Math.round(a[1] + (b[1] - a[1]) * f)
                var g = Math.round(a[2] + (b[2] - a[2]) * f)
                var bl = Math.round(a[3] + (b[3] - a[3]) * f)
                return "rgb(" + r + "," + g + "," + bl + ")"
            }
        }
        return "rgb(20,180,40)"
    }

    function gcsMandala() {
        var gcs = apx.fleet.gcs
        return (gcs && gcs.mandala) ? gcs.mandala : null
    }

    function setAntenna(az, el) {
        try {
            var m = gcsMandala()
            if (!m)
                return
            if (az !== undefined)
                m.cmd.ats.yaw.value = az
            if (el !== undefined)
                m.cmd.ats.pitch.value = el
        } catch (e) {}
    }

    // Manual mode: add to current cmd value
    function nudgeCmd(field, d) {
        try {
            var m = gcsMandala()
            if (!m)
                return
            var f = field === "yaw" ? m.cmd.ats.yaw : m.cmd.ats.pitch
            f.value = f.value + d
        } catch (e) {}
    }

    // Bias: works in any mode
    function nudgeBias(field, d) {
        var f = field === "yaw" ? biasYawFact : biasPitchFact
        if (f)
            f.value = f.value + d
    }

    // Set bias as offset between current pointing and max RSSI point
    function setBiasFromMax() {
        if (maxRssi <= 0)
            return
        if (biasYawFact)
            biasYawFact.value = biasYawFact.value + deltaAz
        if (biasPitchFact)
            biasPitchFact.value = biasPitchFact.value + deltaEl
    }

    function goToMax() {
        if (maxRssi <= 0)
            return
        setAntenna(maxRssiAz, maxRssiEl)
        dataCollectionPaused = true
        pauseTimer.restart()
    }

    // Send scan sector to the modem; scanning itself runs on board
    function startScan() {
        // start with a fresh map so the max reflects this scan only
        clearData()
        try {
            ats.sendSearch(scanAzRange, scanElRange, 1)
        } catch (e) {}
    }

    function clearData() {
        rssiData = ({})
        maxRssi = 0
        maxRssiAz = 0
        maxRssiEl = 0
        heatmap.repaint()
    }

    Timer {
        id: pauseTimer
        interval: 5000
        repeat: false
        onTriggered: rssiPage.dataCollectionPaused = false
    }

    // Update data from mandala
    Timer {
        interval: 100
        repeat: true
        running: true

        onTriggered: {
            var yaw = 0
            var pitch = 0
            var rssi = 0
            try {
                var m = rssiPage.gcsMandala()
                if (m) {
                    yaw = m.est.ats.yaw.value
                    pitch = m.est.ats.pitch.value
                    rssi = m.sns.com.rss.value
                    rssiPage.modeText = m.cmd.ats.mode.text
                    rssiPage.cmdYaw = m.cmd.ats.yaw.value
                    rssiPage.cmdPitch = m.cmd.ats.pitch.value
                }
            } catch (e) {}

            // LOS slant range GCS - UAV
            try {
                var gcs = apx.fleet.gcs
                var unit = apx.fleet.current
                if (gcs && unit && unit !== gcs) {
                    var d = gcs.coordinate.distanceTo(unit.coordinate)
                    var dh = unit.coordinate.altitude - gcs.coordinate.altitude
                    rssiPage.losDistance = Math.sqrt(d * d + dh * dh)
                } else {
                    rssiPage.losDistance = 0
                }
            } catch (e) {}

            if (yaw === 0 && pitch === 0 && rssi === 0)
                return

            rssiPage.currentYaw = yaw < 0 ? yaw + 360 : yaw
            rssiPage.currentPitch = pitch
            rssiPage.currentRssi = rssi

            if (!rssiPage.dataCollectionPaused) {
                var az = Math.round(rssiPage.currentYaw / rssiPage.binSize) * rssiPage.binSize
                var el = Math.round(pitch / rssiPage.binSize) * rssiPage.binSize
                if (az >= rssiPage.azMin && az <= rssiPage.azMax
                        && el >= rssiPage.elMin && el <= rssiPage.elMax) {
                    var key = az + "_" + el
                    var newData = rssiPage.rssiData
                    newData[key] = rssi
                    rssiPage.rssiData = newData

                    // Track maximum
                    if (rssi > rssiPage.maxRssi) {
                        rssiPage.maxRssi = rssi
                        rssiPage.maxRssiAz = az
                        rssiPage.maxRssiEl = el
                    }
                }
            }

            heatmap.repaint()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // Left: chart
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Signal scan")
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: rssiPage.maxRssi > 0
                    text: "Max: " + rssiPage.maxRssi.toFixed(0) + "% @ Az "
                          + rssiPage.maxRssiAz.toFixed(0) + "° / El "
                          + rssiPage.maxRssiEl.toFixed(0) + "°"
                    color: "#44ff44"
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            HeatmapView {
                id: heatmap
                page: rssiPage
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // Right: parameters and control panels
        ColumnLayout {
            Layout.preferredWidth: 280
            Layout.maximumWidth: 280
            Layout.fillHeight: true
            spacing: 6

            ParamsPanel { page: rssiPage }
            ModePanel { page: rssiPage }
            BiasPanel { page: rssiPage }
            ManualPanel { page: rssiPage }
            ScanPanel { page: rssiPage }

            Item { Layout.fillHeight: true }

            ToolBtn {
                Layout.fillWidth: true
                text: qsTr("Clear data")
                onClicked: rssiPage.clearData()
            }
        }
    }
}
