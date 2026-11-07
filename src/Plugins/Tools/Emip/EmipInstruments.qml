import QtQuick

Canvas {
    id: gauges
    objectName: "emipInstruments"
    property var telemetry: null
    property var config: null
    // GCS fonts: the package used Arial / Andale Mono which are not installed
    readonly property string textFamily: apx.font_narrow(12).family
    readonly property string monoFamily: apx.font_fixed(12).family
    readonly property real pumpVoltage: telemetry ? telemetry.pumpVoltage : NaN
    readonly property int pumpLevel: telemetry ? telemetry.pumpMonitor.level : 0
    readonly property string pumpMessage: telemetry ? telemetry.pumpMonitor.message : ""
    onPumpVoltageChanged: requestPaint()
    onPumpLevelChanged: requestPaint()
    onPumpMessageChanged: requestPaint()
    readonly property real rpm: telemetry ? telemetry.rpm : NaN
    readonly property real percent: telemetry ? telemetry.rpmPercent : NaN
    readonly property real egt: telemetry ? telemetry.egt : NaN
    readonly property real fuel1: telemetry ? telemetry.fuel1 : NaN
    readonly property real fuel2: telemetry ? telemetry.fuel2 : NaN
    readonly property real fuel3: telemetry ? telemetry.fuel3 : NaN
    readonly property real total: telemetry ? telemetry.fuelTotal : NaN
    readonly property real rpmWidth: width * 0.35
    readonly property real rpmCenterX: rpmWidth * 0.23
    readonly property real rpmCenterY: height * 0.56
    readonly property real rpmRadius: Math.min(height * 0.35, rpmWidth - rpmCenterX - 16)
    Component.onCompleted: requestPaint()

    EmipNeedle {
        x: gauges.rpmCenterX - gauges.rpmRadius
        y: gauges.rpmCenterY - gauges.rpmRadius
        width: gauges.rpmRadius * 2
        height: width
        value: gauges.percent
        minimum: gauges.config ? gauges.config.rpmMin : 25
        maximum: gauges.config ? gauges.config.rpmMax : 115
    }
    onRpmChanged: requestPaint()
    onPercentChanged: requestPaint()
    onEgtChanged: requestPaint()
    onFuel1Changed: requestPaint()
    onFuel2Changed: requestPaint()
    onFuel3Changed: requestPaint()
    onTotalChanged: requestPaint()
    onConfigChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()


    function display(value, digits) { return isFinite(value) ? value.toFixed(digits) : "—" }

    onPaint: {
        if (!config || width <= 0 || height <= 0) return
        var c = getContext("2d")
        c.reset()
        var w = width, h = height
        var white = "#e5e5e5", grey = "#808280", green = "#4cab50"
        var yellow = "#fafe86", red = "#ff0000"
        var fs = Math.max(10, Math.min(19, w / 35, h / 15))
        function line(x1, y1, x2, y2, color, thickness) {
            c.strokeStyle = color; c.lineWidth = thickness || 1
            c.beginPath(); c.moveTo(x1, y1); c.lineTo(x2, y2); c.stroke()
        }
        function text(value, x, y, size, color, align, bold, family) {
            c.font = (bold ? "bold " : "") + Math.round(size) + "px '" + (family || textFamily) + "'"
            c.fillStyle = color || white
            c.textAlign = align || "center"; c.textBaseline = "middle"
            c.fillText(String(value), x, y)
        }
        function frame(x, ww, title) {
            var left=x+4, right=x+ww-5, top=5, bottom=h-5, r=6
            c.strokeStyle="#222222"; c.lineWidth=1
            c.beginPath(); c.moveTo(left+r,top);c.lineTo(right-r,top)
            c.quadraticCurveTo(right,top,right,top+r);c.lineTo(right,bottom-r)
            c.quadraticCurveTo(right,bottom,right-r,bottom);c.lineTo(left+r,bottom)
            c.quadraticCurveTo(left,bottom,left,bottom-r);c.lineTo(left,top+r)
            c.quadraticCurveTo(left,top,left+r,top);c.stroke()
            if (!title) return
            c.font="bold " + Math.round(fs) + "px '" + textFamily + "'"
            var tw=c.measureText(title).width
            c.fillStyle="#000000";c.fillRect(x+ww/2-tw/2-7,0,tw+14,fs)
            text(title,x+ww/2,fs*0.45,fs,"#808280","center",true)
        }
        function clamp(n, low, high) { return Math.max(low, Math.min(high, n)) }

        var rpmW = w * 0.35, egtW = w * 0.19, fuelX = rpmW + egtW
        frame(0, rpmW + egtW, "RPM")
        var pumpColor=pumpLevel===3 ? red : pumpLevel===2 ? "#ffcf00" : white
        if (pumpMessage) text(pumpMessage,rpmW/2,fs*1.8,fs*0.7,pumpColor,"center",true)
        text("EGT", rpmW + egtW * 0.22, fs * 1.7, fs, white, "center", true)
        frame(fuelX, w - fuelX, "")
        text(display(total, 1) + " L", fuelX + (w - fuelX) / 2, fs * 1.6, fs * 1.1, grey, "center", true)

        // Side-facing tachometer: low at lower right, high at upper right.
        var cx = rpmCenterX, cy = rpmCenterY
        var radius = rpmRadius
        var pumpX = Math.max(fs * 2, cx * 0.7)
        text("PUMP",pumpX,cy-radius*0.60,fs*0.6,grey,"center",true)
        text(display(pumpVoltage,1)+" V",pumpX,cy-radius*0.42,fs*0.95,pumpColor,"center",true)
        function angle(v) { return (60 - (v - config.rpmMin) / (config.rpmMax - config.rpmMin) * 150) * Math.PI / 180 }
        function rpmColor(v) {
            if (v <= config.rpmLowRed || v >= config.rpmHighRed) return red
            if (v <= config.rpmLowYellow || v >= config.rpmHighYellow) return yellow
            return green
        }
        // The original EMIP dial includes solid outer arcs at the red limits.
        c.strokeStyle=red; c.lineWidth=3
        c.beginPath();c.arc(cx,cy,radius,angle(config.rpmHighRed),angle(config.rpmMax),true);c.stroke()
        c.beginPath();c.arc(cx,cy,radius,angle(config.rpmMin),angle(config.rpmLowRed),true);c.stroke()
        for (var v = config.rpmMin; v <= config.rpmMax; v += 3) {
            var a = angle(v), major = (v - config.rpmMin) % 15 === 0
            var tick = major ? radius * 0.09 : radius * 0.035
            line(cx + Math.cos(a) * (radius - tick), cy + Math.sin(a) * (radius - tick),
                 cx + Math.cos(a) * radius, cy + Math.sin(a) * radius, rpmColor(v), major ? 3 : 2)
            if (major)
                text(v, cx + Math.cos(a) * (radius - fs * 1.05), cy + Math.sin(a) * (radius - fs * 1.05), fs * 1.0, white)
        }

        text(display(rpm, 0), cx + fs * 0.1, h - fs * 1.5, fs * 1.3, grey, "center", true)

        var top = fs * 3, bottom = h - fs * 3
        var axisH = Math.max(1, bottom - top)
        // EGT with pointer, ticks and numeric indication.
        var ex = rpmW + egtW * 0.22
        function ey(v) { return bottom - (v - config.egtMin) / (config.egtMax - config.egtMin) * axisH }
        for (v = config.egtMin; v <= config.egtMax; v += 25) {
            var eMajor = (v - config.egtMin) % 150 === 0
            var eColor = v <= config.egtLow || v >= config.egtHigh ? red : white
            line(ex, ey(v), ex + (eMajor ? fs * 0.8 : fs * 0.35), ey(v), eColor, eMajor ? 2 : 1)
            if (eMajor) text(v, ex - fs * 0.2, ey(v), fs * 0.98, eColor, "right")
        }
        if (isFinite(egt)) {
            var ePos = ey(clamp(egt, config.egtMin, config.egtMax))
            c.fillStyle = egt <= config.egtLow || egt >= config.egtHigh ? red : "#0f446e"
            var barX = ex + fs * 0.85
            var barWidth = Math.max(2, Math.min(fs * 2.1, fuelX - 12 - barX))
            c.fillRect(barX,ePos,barWidth,bottom-ePos)
        }
        text(display(egt, 0), rpmW + egtW * 0.30, h - fs * 1.3, fs * 1.5,
             !isFinite(egt) ? grey : (egt < config.egtLow || egt > config.egtHigh ? red : white))

        // One shared scale for three identical 16.7 L tanks, integer graduations.
        var maxLitres = config.fuelScaleMax
        var scaleX = fuelX + fs * 2, laneX = scaleX + fs * 1.2
        var laneW = (w - laneX - fs * 0.4) / 3
        function fy(litres) { return bottom - litres / maxLitres * axisH }
        for (v = 0; v <= maxLitres; ++v) {
            line(scaleX, fy(v), scaleX + (v % 4 === 0 ? fs * 0.55 : fs * 0.3), fy(v), white, 1)
            if (v % 4 === 0)
                text(v, scaleX - fs * 0.3, fy(v), fs * 0.92, white, "right")
        }
        var fuels = [fuel1, fuel2, fuel3]
        for (var i = 0; i < 3; ++i) {
            var bx = laneX + laneW * (i + 0.5), bw = Math.max(5, Math.min(fs * 2.3, laneW * 0.60))
            var cap = config.tankCapacities[i], capY = fy(Math.min(cap, maxLitres))
            c.fillStyle = "#000000"; c.fillRect(bx - bw / 2, capY, bw, bottom - capY)
            if (isFinite(fuels[i])) {
                var fillY = fy(clamp(fuels[i], 0, maxLitres))
                c.fillStyle = fuels[i] < config.tankLow[i] ? red : "#0f446e"
                c.fillRect(bx - bw / 2, fillY, bw, bottom - fillY)
            }

            text(display(fuels[i], 1), bx, h - fs * 1.6, Math.min(fs * 1.25, laneW / 3.2),
                 !isFinite(fuels[i]) ? grey : (fuels[i] < config.tankLow[i] ? red : white), "center", true, monoFamily)
        }
    }
}
