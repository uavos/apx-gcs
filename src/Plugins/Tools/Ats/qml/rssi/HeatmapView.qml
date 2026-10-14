import QtQuick
import QtQuick.Layouts

import Apx.Common

RowLayout {
    id: view

    // RssiWindow instance
    required property var page

    spacing: 0

    function cellsChanged() {
        cells.requestPaint()
    }

    function repaintAll() {
        cells.full = true
        cells.requestPaint()
        overlay.requestPaint()
    }

    // chart geometry
    readonly property real marginLeft: Style.fontSize * 3
    readonly property real marginRight: 10
    readonly property real marginTop: Style.fontSize * 1.5
    readonly property real marginBottom: Style.fontSize * 2.5
    readonly property real chartWidth: chart.width - marginLeft - marginRight
    readonly property real chartHeight: chart.height - marginTop - marginBottom
    readonly property real cellW: chartWidth / page.azCells
    readonly property real cellH: chartHeight / (page.elCells - 1)
    readonly property string axisFont: Math.round(Style.fontSize * 0.7) + "px sans-serif"

    function xPos(az) {
        return marginLeft + az / page.azCells * chartWidth
    }
    function yPos(el) {
        return marginTop + chartHeight - el / (page.elCells - 1) * chartHeight
    }

    function paintCell(ctx, idx) {
        var az = idx % page.azCells
        var el = (idx - az) / page.azCells
        ctx.fillStyle = page.colorFor(page.cells[idx] - 1)
        ctx.fillRect(xPos(az) - cellW / 2, yPos(el) - cellH / 2, cellW + 0.5, cellH + 0.5)
    }

    Item {
        id: chart
        Layout.fillWidth: true
        Layout.fillHeight: true

        // heatmap cells: the canvas keeps its content, new cells are painted on top
        Canvas {
            id: cells
            anchors.fill: parent
            renderTarget: Canvas.Image

            property bool full: true

            onWidthChanged: { full = true; requestPaint() }
            onHeightChanged: { full = true; requestPaint() }

            onPaint: {
                var ctx = getContext("2d")
                if (full) {
                    full = false
                    ctx.reset()
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = "#081c56"
                    ctx.fillRect(view.marginLeft, view.marginTop, view.chartWidth, view.chartHeight)
                    // edge cells are centered on the chart border, keep them inside
                    ctx.beginPath()
                    ctx.rect(view.marginLeft, view.marginTop, view.chartWidth, view.chartHeight)
                    ctx.clip()
                    var data = view.page.cells
                    for (var i = 0; i < data.length; i++)
                        if (data[i] > 0)
                            view.paintCell(ctx, i)
                } else {
                    var pending = view.page.pendingCells
                    for (var j = 0; j < pending.length; j++)
                        view.paintCell(ctx, pending[j])
                }
                view.page.pendingCells.length = 0
            }
        }

        // grid, axes and markers, repainted when the antenna or the maximum moves
        Canvas {
            id: overlay
            anchors.fill: parent
            renderTarget: Canvas.Image

            readonly property real cx: view.xPos(view.page.currentYaw)
            readonly property real cy: view.yPos(Math.max(0, Math.min(view.page.elCells - 1, view.page.currentPitch)))
            readonly property real mx: view.xPos(view.page.maxRssiAz)
            readonly property real my: view.yPos(view.page.maxRssiEl)
            readonly property bool hasMax: view.page.maxRssi > 0

            onCxChanged: requestPaint()
            onCyChanged: requestPaint()
            onMxChanged: requestPaint()
            onMyChanged: requestPaint()
            onHasMaxChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var page = view.page
                var ctx = getContext("2d")
                ctx.reset()
                ctx.clearRect(0, 0, width, height)

                // grid
                ctx.strokeStyle = "rgba(160,190,255,0.25)"
                ctx.lineWidth = 1
                var a, e, x, y
                for (a = 0; a <= page.azCells; a += 15) {
                    x = view.xPos(a)
                    ctx.beginPath()
                    ctx.moveTo(x, view.marginTop)
                    ctx.lineTo(x, view.marginTop + view.chartHeight)
                    ctx.stroke()
                }
                for (e = 0; e < page.elCells; e += 10) {
                    y = view.yPos(e)
                    ctx.beginPath()
                    ctx.moveTo(view.marginLeft, y)
                    ctx.lineTo(view.marginLeft + view.chartWidth, y)
                    ctx.stroke()
                }

                // axis labels
                ctx.fillStyle = "#a0b8e0"
                ctx.font = view.axisFont
                ctx.textAlign = "center"
                for (a = 0; a <= page.azCells; a += 45)
                    ctx.fillText(a + "°", view.xPos(a), view.marginTop + view.chartHeight + Style.fontSize)
                ctx.fillText(qsTr("Azimuth") + " (°)",
                             view.marginLeft + view.chartWidth / 2, view.marginTop + view.chartHeight + Style.fontSize * 2)

                ctx.textAlign = "right"
                for (e = 0; e < page.elCells; e += 10)
                    ctx.fillText(e + "°", view.marginLeft - 5, view.yPos(e) + 4)
                ctx.textAlign = "left"
                ctx.fillText(qsTr("Elevation") + " (°)", 2, view.marginTop - 8)

                // max marker: circle with dot
                if (hasMax) {
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

                // current antenna position: X marker
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
    }

    // legend: gradient color bar
    Canvas {
        id: legend
        Layout.preferredWidth: Style.fontSize * 4.5
        Layout.fillHeight: true
        renderTarget: Canvas.Image

        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var top = view.marginTop + Style.fontSize * 1.5
            var barH = height - top - view.marginBottom - 10
            var barW = 16
            var barX = 8

            ctx.fillStyle = "#a0b8e0"
            ctx.font = view.axisFont
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
