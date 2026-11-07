import QtQuick

QtObject {
    id: data
    property var unit: null
    property var config: null
    readonly property var mandala: unit ? unit.mandala : null
    readonly property bool available: !!unit && (unit.streamType > 0 || unit.isReplay === true)

    function find(paths) {
        if (!mandala || !paths) return null
        for (var i = 0; i < paths.length; ++i) {
            var f = mandala.fact(paths[i], true)
            if (f) return f
        }
        return null
    }
    function numeric(f) {
        if (!available || !f || f.value === null || f.value === undefined || f.value === "") return NaN
        var n = Number(f.value)
        return isFinite(n) ? n : NaN
    }
    function on(f) {
        if (!available || !f) return null
        var text = String(f.text).toLowerCase()
        if (["on", "yes", "released"].indexOf(text) >= 0) return true
        if (["off", "no", "locked"].indexOf(text) >= 0) return false
        var n = numeric(f)
        return isFinite(n) ? n !== 0 : null
    }
    function released(f) {
        if (!available || !f) return null
        var text = String(f.text).toLowerCase()
        if (text === "released" || text === "off") return true
        if (text === "locked" || text === "on") return false
        return null
    }
    function level(known, active, severity) { return !known ? -1 : (active ? severity : 0) }

    readonly property var pumpVoltageFact: find(config ? config.pumpVoltagePaths : [])
    readonly property var throttleFeedbackFact: find(config ? config.throttleFeedbackPaths : [])
    readonly property var engineStatusFact: find(config ? config.engineStatusPaths : [])
    readonly property real pumpVoltage: numeric(pumpVoltageFact)
    readonly property real throttleFeedback: numeric(throttleFeedbackFact)
    readonly property real engineStatus: numeric(engineStatusFact)
    readonly property var pumpMonitor: EmipPumpMonitor {
        unit: data.unit
        config: data.config
        available: data.available
        voltage: data.pumpVoltage
        rpm: data.rpm
        throttle: data.throttleFeedback
        engineStatus: data.engineStatus
    }
    readonly property var rpmFact: find(config ? config.rpmPaths : [])
    readonly property var egtFact: find(config ? config.egtPaths : [])
    readonly property var fuelTotalFact: find(config ? config.fuelTotalPaths : [])
    readonly property var fuel1Fact: find(config ? config.fuel1Paths : [])
    readonly property var fuel2Fact: find(config ? config.fuel2Paths : [])
    readonly property var fuel3Fact: find(config ? config.fuel3Paths : [])
    readonly property var relay1Fact: find(config ? config.relay1Paths : [])
    readonly property var relay2Fact: find(config ? config.relay2Paths : [])
    readonly property var relay3Fact: find(config ? config.relay3Paths : [])
    readonly property var satcomFact: find(config ? config.satcomPaths : [])
    readonly property var batteryFact: find(config ? config.batteryPaths : [])
    readonly property real rpm: numeric(rpmFact) * (config ? config.rpmMultiplier : 1)
    readonly property real rpmPercent: config && config.rpm100 > 0 ? rpm * 100 / config.rpm100 : NaN
    readonly property real egt: numeric(egtFact) * (config ? config.egtMultiplier : 1)
    readonly property real fuel1: numeric(fuel1Fact)
    readonly property real fuel2: numeric(fuel2Fact)
    readonly property real fuel3: numeric(fuel3Fact)
    readonly property real measuredFuelTotal: numeric(fuelTotalFact)
    readonly property real fuelTotal: isFinite(measuredFuelTotal) ? measuredFuelTotal : fuel1 + fuel2 + fuel3
    readonly property var relay1: on(relay1Fact)
    readonly property var relay2: on(relay2Fact)
    readonly property var relay3: released(relay3Fact)
    readonly property var satcom: on(satcomFact)
    readonly property real battery: numeric(batteryFact)
    readonly property real batteryCellVoltage: config && config.batterySeries > 0 ? battery / config.batterySeries : NaN

    property int fuelWarningLevel: 0
    property var fuelWarningUnit: null
    function updateFuelWarning() {
        if (fuelWarningUnit !== unit) {
            fuelWarningLevel = 0
            fuelWarningUnit = unit
        }
        // Retain the latch through a temporary gap for the same unit.
        // The indicator itself shows unknown while data is unavailable.
        if (!available || !config || !isFinite(fuelTotal)) return
        fuelWarningLevel = nextWarningLevel(fuelTotal, config.fuelTotalLow,
            config.fuelTotalCritical, config.fuelTotalHysteresis, fuelWarningLevel)
    }
    function nextWarningLevel(value, warning, critical, hysteresis, previous) {
        if (value <= critical + 1e-9) return 3
        if (previous === 3 && value < critical + hysteresis - 1e-9) return 3
        if (value <= warning + 1e-9) return 2
        if (previous > 0 && value < warning + hysteresis - 1e-9) return 2
        return 0
    }
    property int batteryWarningLevel: 0
    property var batteryWarningUnit: null
    function updateBatteryWarning() {
        if (batteryWarningUnit !== unit) {
            batteryWarningLevel = 0
            batteryWarningUnit = unit
        }
        if (!available || !config || !isFinite(batteryCellVoltage)) return
        batteryWarningLevel = nextWarningLevel(batteryCellVoltage,
            config.batteryWarningCellVoltage, config.batteryCriticalCellVoltage,
            config.batteryCellHysteresis, batteryWarningLevel)
    }
    onBatteryCellVoltageChanged: updateBatteryWarning()

    onFuelTotalChanged: updateFuelWarning()
    onUnitChanged: {
        fuelWarningUnit = null
        fuelWarningLevel = 0
        batteryWarningUnit = null
        batteryWarningLevel = 0
        Qt.callLater(updateFuelWarning)
        Qt.callLater(updateBatteryWarning)
    }
    onAvailableChanged: { updateFuelWarning(); updateBatteryWarning() }
    Component.onCompleted: { updateFuelWarning(); updateBatteryWarning() }

    // -1 unknown, 0 inactive, 1 green, 2 amber, 3 red.
    readonly property var annunciators: [
        {label: "LOW FUEL", state: isFinite(fuelTotal) ? fuelWarningLevel : -1},
        {label: "LOW BAT", state: isFinite(batteryCellVoltage) ? batteryWarningLevel : -1},
        {label: "SATCOM", state: level(satcom !== null, satcom, 1)}
    ]
}
