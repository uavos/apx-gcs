import QtQuick
import QtQuick.Layouts

RowLayout {
    id: view

    // RssiWindow instance
    required property var page

    spacing: 0

    function repaint() {
        canvas.requestPaint()
    }

    Canvas {
        id: canvas
        Layout.fillWidth: true
        Layout.fillHeight: true

        Component.onCompleted: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        property real marginLeft: 45
        property real marginRight: 10
        property real marginTop: 20
        property real marginBottom: 35
        property real chartWidth: width - marginLeft - marginRight
        property real chartHeight: height - marginTop - marginBottom

        function xPos(az) {
            return marginLeft + ((az - view.page.azMin) / (view.page.azMax - view.page.azMin)) * chartWidth
        }
        function yPos(el) {
            return marginTop + chartHeight
                    - ((el - view.page.elMin) / (view.page.elMax - view.page.elMin)) * chartHeight
        }

        onPaint: {
            var page = view.page
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            // Background (no-data)
            ctx.fillStyle = "#081c56"
            ctx.fillRect(marginLeft, marginTop, chartWidth, chartHeight)

            // Heatmap cells
            var cellW = (page.binSize / (page.azMax - page.azMin)) * chartWidth
            var cellH = (page.binSize / (page.elMax - page.elMin)) * chartHeight
            var data = page.rssiData
            var keys = Object.keys(data)
            for (var i = 0; i < keys.length; i++) {
                var parts = keys[i].split("_")
                var az = parseFloat(parts[0])
                var el = parseFloat(parts[1])
                ctx.fillStyle = page.colorFor(data[keys[i]])
                ctx.fillRect(xPos(az) - cellW / 2, yPos(el) - cellH / 2,
                             cellW + 0.5, cellH + 0.5)
            }

            // Grid
            ctx.strokeStyle = "rgba(160,190,255,0.25)"
            ctx.lineWidth = 1
            var a, e, x, y
            for (a = page.azMin; a <= page.azMax; a += 15) {
                x = xPos(a)
                ctx.beginPath()
                ctx.moveTo(x, marginTop)
                ctx.lineTo(x, marginTop + chartHeight)
                ctx.stroke()
            }
            for (e = page.elMin; e <= page.elMax; e += 10) {
                y = yPos(e)
                ctx.beginPath()
                ctx.moveTo(marginLeft, y)
                ctx.lineTo(marginLeft + chartWidth, y)
                ctx.stroke()
            }

            // Axis labels
            ctx.fillStyle = "#a0b8e0"
            ctx.font = "10px sans-serif"
            ctx.textAlign = "center"
            for (a = page.azMin; a <= page.azMax; a += 45)
                ctx.fillText(a + "°", xPos(a), marginTop + chartHeight + 15)
            ctx.fillText(qsTr("Azimuth") + " (°)",
                         marginLeft + chartWidth / 2, marginTop + chartHeight + 30)

            ctx.textAlign = "right"
            for (e = page.elMin; e <= page.elMax; e += 10)
                ctx.fillText(e + "°", marginLeft - 5, yPos(e) + 4)
            ctx.textAlign = "left"
            ctx.fillText(qsTr("Elevation") + " (°)", 2, marginTop - 8)

            // Max marker: circle with dot
            if (page.maxRssi > 0) {
                var mx = xPos(page.maxRssiAz)
                var my = yPos(page.maxRssiEl)
                ctx.strokeStyle = "#ffffff"
                ctx.lineWidth = 2.5
                ctx.beginPath()
                ctx.arc(mx, my, 9, 0, Math.PI * 2)
                ctx.stroke()
                ctx.fillStyle = "#ffffff"
                ctx.beginPath()
                ctx.arc(mx, my, 3.5, 0, Math.PI * 2)
                ctx.fill()
            }

            // Current antenna position: X marker
            var cx = xPos(Math.max(page.azMin, Math.min(page.azMax, page.currentYaw)))
            var cy = yPos(Math.max(page.elMin, Math.min(page.elMax, page.currentPitch)))

            // Deviation line to max point while scanning
            if (page.scanning && page.maxRssi > 0) {
                ctx.strokeStyle = "rgba(255,255,255,0.6)"
                ctx.lineWidth = 1.5
                ctx.setLineDash([5, 4])
                ctx.beginPath()
                ctx.moveTo(cx, cy)
                ctx.lineTo(xPos(page.maxRssiAz), yPos(page.maxRssiEl))
                ctx.stroke()
                ctx.setLineDash([])
            }
            ctx.strokeStyle = "#ffffff"
            ctx.lineWidth = 3.5
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.moveTo(cx - 7, cy - 7)
            ctx.lineTo(cx + 7, cy + 7)
            ctx.moveTo(cx + 7, cy - 7)
            ctx.lineTo(cx - 7, cy + 7)
            ctx.stroke()
        }
    }

    // Legend: gradient color bar
    Canvas {
        id: legend
        Layout.preferredWidth: 70
        Layout.fillHeight: true

        Component.onCompleted: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var top = canvas.marginTop + 20
            var barH = height - top - canvas.marginBottom - 10
            var barW = 16
            var barX = 8

            ctx.fillStyle = "#a0b8e0"
            ctx.font = "10px sans-serif"
            ctx.textAlign = "left"
            ctx.fillText(qsTr("Signal") + " (%)", barX, top - 8)

            for (var i = 0; i < barH; i++) {
                var v = 100 * (1 - i / barH)
                ctx.fillStyle = view.page.colorFor(v)
                ctx.fillRect(barX, top + i, barW, 1.5)
            }
            ctx.strokeStyle = "rgba(160,190,255,0.4)"
            ctx.lineWidth = 1
            ctx.strokeRect(barX, top, barW, barH)

            ctx.fillStyle = "#a0b8e0"
            for (var p = 0; p <= 100; p += 20) {
                var y = top + barH * (1 - p / 100)
                ctx.fillText(p.toString(), barX + barW + 5, y + 3)
            }
        }
    }
}
