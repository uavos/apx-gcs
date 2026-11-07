import QtQuick

QtObject {
    id: monitor
    property var unit: null
    property var config: null
    property bool available: false
    property real rpm: NaN
    property real voltage: NaN
    property real throttle: NaN
    property real engineStatus: NaN
    property bool automatic: true
    property var state: createState()
    readonly property int level: state.level
    readonly property string message: state.message
    function clockNow() {
        var player=unit && unit.isReplay && unit.telemetry ? unit.telemetry.player : null
        if (player && player.time && isFinite(Number(player.time.value)))
            return Number(player.time.value)
        return Date.now()
    }
    function reset() { state=createState() }
    function step(now) {
        if (!config) return
        state=update(state,{time:now,available:available,rpm:rpm,
            voltage:voltage,throttle:throttle,status:engineStatus},config)
    }
    onUnitChanged: reset()
    onAvailableChanged: if (!available) reset()
    property Timer timer: Timer {
        interval: 250
        repeat: true
        running: monitor.available && monitor.automatic
        onTriggered: monitor.step(monitor.clockNow())
    }

    // pump voltage monitor logic

    function createState() {
        return {lastTime: NaN, history: [], armed: false, highSince: NaN,
                stopSince: NaN, maxSince: NaN, clearSince: NaN,
                level: 0, message: "", expectedRpm: NaN, expectedThrottle: NaN}
    }
    function interpolate(points, value) {
        if (value <= points[0][0]) return points[0][1]
        for (var i=1; i<points.length; ++i) {
            if (value <= points[i][0]) {
                var a=points[i-1], b=points[i]
                return a[1]+(b[1]-a[1])*(value-a[0])/(b[0]-a[0])
            }
        }
        return points[points.length-1][1]
    }
    function update(previous, sample, c) {
        var s=Object.assign({},previous)
        var now=sample.time
        if (!isFinite(now) || !sample.available || !isFinite(sample.rpm) || !isFinite(sample.voltage))
            return createState()
        if (isFinite(s.lastTime) && (now < s.lastTime || now-s.lastTime > c.pumpDataGapMs))
            s=createState()
        if (now===s.lastTime) return s
        s.lastTime=now
        var running=c.pumpRunningStatuses.indexOf(sample.status)>=0
        if (running && sample.rpm>=c.pumpRunningRpm) s.armed=true
        var h=s.history.filter(function(v){return now-v.time<=c.pumpStableWindowMs})
        h.push({time:now,rpm:sample.rpm,throttle:sample.throttle})
        s.history=h
        var rpms=h.map(function(v){return v.rpm}), throttles=h.map(function(v){return v.throttle})
        var stable=h.length>1 && now-h[0].time>=c.pumpStableWindowMs*0.8
            && Math.max.apply(null,rpms)-Math.min.apply(null,rpms)<=c.pumpStableRpmRange
            && throttles.every(isFinite)
            && Math.max.apply(null,throttles)-Math.min.apply(null,throttles)<=c.pumpStableThrottleRange
        s.expectedRpm=interpolate(c.pumpRpmReference,sample.rpm)
        s.expectedThrottle=isFinite(sample.throttle) ? interpolate(c.pumpThrottleReference,sample.throttle) : NaN
        var mismatch=running && stable && sample.rpm>=c.pumpRunningRpm
            && sample.voltage>s.expectedRpm+c.pumpRpmMarginVolts
            && sample.voltage>s.expectedThrottle+c.pumpThrottleMarginVolts
        var absoluteHigh=running && sample.voltage>c.pumpAbsoluteHigh
        var noPump=running && stable && sample.rpm>=c.pumpRunningRpm && sample.voltage<=c.pumpStoppedVoltage
        var high=mismatch || absoluteHigh || noPump
        if (high) {
            if (!isFinite(s.highSince)) s.highSince=now
            s.clearSince=NaN
            if (now-s.highSince>=c.pumpWarningDelayMs) {
                s.level=3
                s.message=noPump ? "NO PUMP" : (absoluteHigh ? "PUMP LIMIT" : "CHECK PUMP")
            }
        } else {
            s.highSince=NaN
            if (s.level===3) {
                if (!isFinite(s.clearSince)) s.clearSince=now
                if (now-s.clearSince<c.pumpClearDelayMs) return s
            }
            s.level=0;s.message="";s.clearSince=NaN
        }
        if (s.level===3) return s
        var stopped=s.armed && sample.rpm<c.pumpStoppedRpm && sample.voltage<=c.pumpStoppedVoltage
        if (stopped) {
            if (!isFinite(s.stopSince)) s.stopSince=now
            if (now-s.stopSince>=c.pumpWarningDelayMs) {s.level=2;s.message="ENGINE STOP"}
        } else {s.stopSince=NaN}
        var maximum=running && sample.rpm>=c.pumpMaxRpm && sample.voltage>=c.pumpMaxVoltage
            && isFinite(sample.throttle) && sample.throttle>=c.pumpMaxThrottle
        if (maximum) {
            if (!isFinite(s.maxSince)) s.maxSince=now
            if (now-s.maxSince>=c.pumpMaxDelayMs) {s.level=2;s.message="MAX"}
        } else {s.maxSince=NaN}
        return s
    }
}
