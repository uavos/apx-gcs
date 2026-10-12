import QtQuick
import QtQuick.Window
import QtQuick.Layouts

import Apx.Common
import APX.Fleet as APX

// RSSI window, loaded by Fact::loadQml as a root object: a Window parented
// to an Item outside of any window would never be shown by Qt
Window {
    id: rssiPage

    // set by the Ats plugin on creation: the plugin itself (apx.tools.ats is not
    // registered yet at that moment), the window toggle fact, the mode fact and the GCS unit
    property var ats
    property var fact
    property var modeFact
    property var unit

    // GCS unit under control
    readonly property var gcs: unit
    readonly property var m: gcs ? gcs.mandala : null
    readonly property bool online: gcs ? gcs.streamType !== APX.PUnit.OFFLINE : false

    // signal percent to dBm, the inverse of the modem scaling:
    // rss = (raw - 120) / (-120 + 55) * 100, raw is -dBm (MHX_RSS_MIN, MHX_RSS_MAX)
    readonly property real rssMinDbm: -120
    readonly property real rssMaxDbm: -55
    function toDbm(percent) {
        return rssMinDbm + percent / 100 * (rssMaxDbm - rssMinDbm)
    }

    // antenna state
    readonly property real currentYaw: m ? (m.est.ats.yaw.value + 360) % 360 : 0
    readonly property real currentPitch: m ? m.est.ats.pitch.value : 0
    readonly property real currentRssi: m ? m.sns.com.rss.value : 0
    readonly property int mode: m ? m.cmd.ats.mode.value : ats_mode_off
    readonly property string modeText: m ? m.cmd.ats.mode.text : ""
    readonly property real cmdYaw: m ? m.cmd.ats.yaw.value : 0
    readonly property real cmdPitch: m ? m.cmd.ats.pitch.value : 0

    // LOS slant range from the GCS to the unit tracked by ATS
    readonly property var tracked: ats ? ats.trackedUnit : null
    readonly property real losDistance: {
        if (!gcs || !tracked || tracked === gcs)
            return 0
        if (!gcs.coordinate.isValid || !tracked.coordinate.isValid)
            return 0
        var d = gcs.coordinate.distanceTo(tracked.coordinate)
        var dh = tracked.coordinate.altitude - gcs.coordinate.altitude
        return Math.sqrt(d * d + dh * dh)
    }

    // heatmap cells, 1 deg bins, stored as rssi + 1 so that 0 means no data
    readonly property int azCells: 360
    readonly property int elCells: 91
    property var cells: new Float32Array(azCells * elCells)
    property var pendingCells: [] // indices to paint on the next frame

    property real maxRssi: 0
    property int maxRssiAz: 0
    property int maxRssiEl: 0

    property bool dataCollectionPaused: false
    readonly property bool collecting: visible && online && !dataCollectionPaused

    title: qsTr("RSSI") + (gcs ? " - " + gcs.title : "")
    width: 800
    height: 450
    minimumWidth: 560
    minimumHeight: 380
    color: "#10141f"
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint | Qt.WindowMinMaxButtonsHint

    // visibility follows the fact; closing only resets the fact to keep the binding
    visible: fact ? fact.value : false
    onClosing: (close) => {
        close.accepted = false
        fact.value = false
    }
    onVisibleChanged: if (visible) heatmap.repaintAll()

    onCurrentYawChanged: sample()
    onCurrentPitchChanged: sample()
    onCurrentRssiChanged: sample()

    function sample() {
        if (!collecting)
            return
        var az = Math.round(currentYaw) % azCells
        var el = Math.round(currentPitch)
        if (el < 0 || el >= elCells)
            return
        var idx = el * azCells + az
        cells[idx] = currentRssi + 1
        pendingCells.push(idx)
        if (currentRssi > maxRssi) {
            maxRssi = currentRssi
            maxRssiAz = az
            maxRssiEl = el
        } else if (az === maxRssiAz && el === maxRssiEl && currentRssi < maxRssi) {
            findMax() // the max cell was measured again with a lower value
        }
        heatmap.cellsChanged()
    }

    // maximum over all measured cells
    function findMax() {
        var best = 0, bestIdx = -1
        for (var i = 0; i < cells.length; i++) {
            if (cells[i] > best) {
                best = cells[i]
                bestIdx = i
            }
        }
        if (bestIdx < 0) {
            maxRssi = 0
            maxRssiAz = 0
            maxRssiEl = 0
            return
        }
        maxRssi = best - 1
        maxRssiAz = bestIdx % azCells
        maxRssiEl = (bestIdx - maxRssiAz) / azCells
    }

    function clearData() {
        cells = new Float32Array(azCells * elCells)
        pendingCells = []
        maxRssi = 0
        maxRssiAz = 0
        maxRssiEl = 0
        heatmap.repaintAll()
    }

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

    // manual control: commands go to the selected GCS unit only
    function setAntenna(az, el) {
        if (!m)
            return
        m.cmd.ats.yaw.value = az
        m.cmd.ats.pitch.value = el
    }

    function nudgeCmd(field, d) {
        if (!m)
            return
        var f = field === "yaw" ? m.cmd.ats.yaw : m.cmd.ats.pitch
        f.value = f.value + d
    }

    function goToMax() {
        if (maxRssi <= 0)
            return
        setAntenna(maxRssiAz, maxRssiEl)
        dataCollectionPaused = true
        pauseTimer.restart()
    }

    Timer {
        id: pauseTimer
        interval: 5000
        onTriggered: rssiPage.dataCollectionPaused = false
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

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
                    font.pixelSize: Style.fontSize * 0.9
                    font.bold: true
                }

                // GCS unit the window works with
                Text {
                    text: rssiPage.gcs ? rssiPage.gcs.title : qsTr("No GCS unit")
                    color: rssiPage.gcs ? "#00ffff" : "#ffaa00"
                    font.pixelSize: Style.fontSize * 0.9
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: rssiPage.maxRssi > 0
                    text: "Max: " + rssiPage.maxRssi.toFixed(0) + "% @ Az "
                          + rssiPage.maxRssiAz + "° / El " + rssiPage.maxRssiEl + "°"
                    color: "#44ff44"
                    font.pixelSize: Style.fontSize * 0.8
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
            Layout.preferredWidth: Style.fontSize * 12
            Layout.maximumWidth: Style.fontSize * 12
            Layout.fillHeight: true
            spacing: 4

            ParamsPanel { page: rssiPage }
            ModePanel { page: rssiPage; enabled: rssiPage.gcs !== null }
            ManualPanel { page: rssiPage }

            ToolBtn {
                Layout.fillWidth: true
                text: qsTr("Clear data")
                onClicked: rssiPage.clearData()
            }

            Item { Layout.fillHeight: true }
        }
    }
}
