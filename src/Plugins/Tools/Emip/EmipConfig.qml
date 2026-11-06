import QtQuick

// Defaults from QT_6/EMIP/Settings.h. First existing path is used.
QtObject {
    property var rpmPaths: ["est.eng.rpm", "sns.env.eng.rpm"]
    property var pumpVoltagePaths: ["est.usr.u9", "est.env.usr.u9"]
    property var throttleFeedbackPaths: ["est.usrc.c3", "est.env.usrc.c3"]
    property var engineStatusPaths: ["est.usrc.c2", "est.env.usrc.c2"]
    property var egtPaths: ["sns.eng.egt", "sns.env.eng.egt"]
    property var fuelTotalPaths: ["est.usr.u8", "est.env.usr.u8"]
    property var fuel1Paths: ["est.usr.u5", "est.env.usr.u5"]
    property var fuel2Paths: ["est.usr.u6", "est.env.usr.u6"]
    property var fuel3Paths: ["est.usr.u7", "est.env.usr.u7"]
    property var relay1Paths: ["ctr.ers.launch", "ctr.env.ers.launch"]
    property var relay2Paths: ["est.usrb.b1", "est.env.usrb.b1"]
    property var relay3Paths: ["ctr.ers.rel", "ctr.env.ers.rel"]
    property var satcomPaths: ["ctr.pwr.satcom", "ctr.env.pwr.satcom"]
    property var batteryPaths: ["sns.pwr.vsys", "sns.env.pwr.vsys", "sns.bat.voltage"]
    property real rpm100: 9600
    property real rpmMin: 25
    property real rpmMax: 115
    property real rpmLowRed: 30
    property real rpmLowYellow: 35
    property real rpmHighYellow: 104
    property real rpmHighRed: 105
    property real egtMin: 350
    property real egtMax: 950
    property real egtLow: 400
    property real egtHigh: 850
    // Live screenshot confirms RPM Fact.value is already in rpm. EGT still uses x10.
    property real rpmMultiplier: 1
    property real egtMultiplier: 10
    property real fuelScaleMax: 16
    property var tankCapacities: [16.7, 16.7, 16.7]
    property var tankLow: [2, 3, 4]
    property real fuelTotalLow: 10
    property real fuelTotalCritical: 5
    property real fuelTotalHysteresis: 2
    property int batterySeries: 4
    // Provisional loaded-voltage thresholds, not a calibrated remaining-time model.
    // NCR18650GA reference curves: 25 C / 2 A; actual average ~1.12 A/cell.
    property real batteryWarningCellVoltage: 3.5
    property real batteryCriticalCellVoltage: 3.3
    property real batteryCellHysteresis: 0.3
    // Provisional reference from 261026_1412_22256E330-GCULV-01.apxtlm.
    property var pumpRpmReference: [[3500,2.8],[4250,3.2],[4750,3.6],[5250,4.0],
        [5750,4.5],[6250,4.8],[6750,5.3],[7250,6.0],[7750,6.5],
        [8250,7.0],[8750,8.0],[9250,9.5],[9500,9.9]]
    property var pumpThrottleReference: [[0,2.8],[100,9.9]]
    property var pumpRunningStatuses: [10,11]
    property real pumpRunningRpm: 2000
    property real pumpRpmMarginVolts: 0.7
    property real pumpThrottleMarginVolts: 0.8
    property real pumpAbsoluteHigh: 10.5
    property real pumpStoppedVoltage: 0.2
    property real pumpStoppedRpm: 800
    property real pumpMaxVoltage: 9.5
    property real pumpMaxRpm: 9000
    property real pumpMaxThrottle: 90
    property real pumpStableRpmRange: 600
    property real pumpStableThrottleRange: 10
    property int pumpStableWindowMs: 3000
    property int pumpWarningDelayMs: 3000
    property int pumpClearDelayMs: 2000
    property int pumpMaxDelayMs: 2000
    property int pumpDataGapMs: 3000
}
