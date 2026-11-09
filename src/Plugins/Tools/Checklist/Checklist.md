# Aircraft check

Pre-flight check of the aircraft. Press **Start** and follow the steps in order:
automatic steps are evaluated by the system, the others wait for the technician
and the operator. A failed step can be repeated with **Retry** or passed over with
**Continue**, the whole procedure with **Restart**.

### Before you begin

- the aircraft is powered and the datalink is up;
- the technician is at the aircraft and can talk to the operator;
- the area around the control surfaces, the propeller and the parachute cover is clear.

Thresholds and signal names used by the steps are in the shared block below,
edit them for the aircraft. The step texts show the current values from that block,
for example battery minimum {{cfg.batteryMin}} V.

```js
// shared block: runs before every step script
var cfg = {
    batteryMin: 15,              // V, sns.pwr.vsys
    fuelMin: 10,                 // %, sns.fuel.level
    gpsMinSats: 6,               // sns.gps.su
    ranges: [                    // variables checked against allowed ranges
        { path: "sns.pwr.vsys", min: 14, max: 17.5 },
        { path: "sns.pwr.vsrv", min: 7, max: 9 },
    ],
    engineStatus: { path: "est.usrc.c2", ok: 1 },   // engine status word and its OK value

    diveAltitudeMin: 1000,       // m
    diveAngleMin: 20,            // deg
    diveAngleMax: 60,            // deg
    params: {                    // node parameter names, set by the flight configuration step
        diveAltitude: "dive_altitude",
        safetyTimeout: "safety_timeout",
        diveAngle: "dive_angle",
    },

    levelMaxDeg: 5,              // roll/pitch to consider the aircraft level
    tiltMinDeg: 10,              // tilt range for the RPV check
    tiltMaxDeg: 30,
    magToleranceDeg: 15,         // magnetometer heading tolerance
    airspeedZeroMax: 3,          // m/s with the pitot cover on
    airspeedBlowMin: 5,          // m/s when blowing into the pitot
    ers: ["ctr.ers.launch", "ctr.ers.rel"],   // parachute locks in opening order (Rel-1, Rel-2, ...)
    pumps: ["ctr.eng.fpump"],                 // fuel pump outputs (Pump 1, 2, 3)
    catapultHeadingToleranceDeg: 10,
    rpmReady: 3000,              // engine rpm for takeoff
}

var deg = 180 / Math.PI
function val(path) { return apx.fleet.current.mandala.fact(path).value }
function set(path, v) { apx.fleet.current.mandala.fact(path).value = v }
function nodes() { return apx.fleet.current.findChild("nodes") }
// set a node parameter by its name in any node, returns false when not found
function setParam(name, value) {
    var n = nodes()
    for (var i = 0; i < n.size; ++i) {
        var f = n.child(i).findChild(name)
        if (!f) continue
        f.value = value
        return true
    }
    return false
}
function shuffle(a) {
    for (var i = a.length - 1; i > 0; --i) {
        var j = Math.floor(Math.random() * (i + 1))
        var t = a[i]; a[i] = a[j]; a[j] = t
    }
    return a
}
```

## System check

The system reads telemetry and checks it against the limits from the shared block.
Problems are listed below the description while the check waits for valid data
(up to 10 s), the step fails with the remaining ones.

| Check | Condition |
|---|---|
| Battery voltage | `sns.pwr.vsys` above {{cfg.batteryMin}} V |
| Fuel level | `sns.fuel.level` above {{cfg.fuelMin}} % |
| Nodes | all nodes are loaded and valid |
| EKF | attitude estimation status is OK |
| Variables | {{cfg.ranges.map(function (r) { return r.path + " in " + r.min + ".." + r.max }).join(", ")}} |
| Engine status | `{{cfg.engineStatus.path}}` equals {{cfg.engineStatus.ok}} |
| GPS | 3D fix and at least {{cfg.gpsMinSats}} used satellites |

```js
function problems() {
    var p = []
    if (!(sns.pwr.vsys >= cfg.batteryMin)) p.push("battery " + sns.pwr.vsys.toFixed(1) + " V < " + cfg.batteryMin + " V")
    if (!(sns.fuel.level >= cfg.fuelMin)) p.push("fuel " + sns.fuel.level + " % < " + cfg.fuelMin + " %")
    if (!nodes().valid) p.push("nodes not loaded")
    if (est.att.status != att_status_ok) p.push("EKF status not OK")
    for (var i = 0; i < cfg.ranges.length; ++i) {
        var r = cfg.ranges[i], v = val(r.path)
        if (!(v >= r.min && v <= r.max)) p.push(r.path + " = " + v + " out of " + r.min + ".." + r.max)
    }
    if (val(cfg.engineStatus.path) != cfg.engineStatus.ok) p.push("engine status " + val(cfg.engineStatus.path))
    if (sns.gps.fix < gps_fix_3D) p.push("no GPS 3D fix")
    if (sns.gps.su < cfg.gpsMinSats) p.push("GPS satellites " + sns.gps.su + " < " + cfg.gpsMinSats)
    return p
}
step.poll(function () {
    var p = problems()
    step.info(p.length ? p.join("\n") : "all checks passed")
    return p.length === 0
}, 3)
```

## Flight configuration

Enter the flight parameters. The values are written to the autopilot nodes
(parameter names from `params` in the shared block) and uploaded.

- **Dive altitude** is limited from below by {{cfg.diveAltitudeMin}} m;
- **Safety timeout** is the return time after a datalink loss, seconds;
- **Dive angle** must be inside {{cfg.diveAngleMin}}..{{cfg.diveAngleMax}} deg.

Guide altitude is set to 0 automatically.

```js
step.form({ "Dive altitude, m:": 1500, "Safety timeout, s:": 60, "Dive angle, deg:": 30 }, function (v) {
    var alt = Number(v["Dive altitude, m"])
    var timeout = Number(v["Safety timeout, s"])
    var angle = Number(v["Dive angle, deg"])
    if (!(alt >= cfg.diveAltitudeMin)) return step.fail("dive altitude " + alt + " m is below " + cfg.diveAltitudeMin + " m")
    if (!(timeout >= 0)) return step.fail("safety timeout must be a positive number")
    if (!(angle >= cfg.diveAngleMin && angle <= cfg.diveAngleMax)) return step.fail("dive angle " + angle + " is out of " + cfg.diveAngleMin + ".." + cfg.diveAngleMax)
    var missing = []
    if (!setParam(cfg.params.diveAltitude, alt)) missing.push(cfg.params.diveAltitude)
    if (!setParam(cfg.params.safetyTimeout, timeout)) missing.push(cfg.params.safetyTimeout)
    if (!setParam(cfg.params.diveAngle, angle)) missing.push(cfg.params.diveAngle)
    if (missing.length) return step.fail("parameters not found in nodes: " + missing.join(", "))
    nodes().findChild("upload").trigger()
    step.pass("dive " + alt + " m / " + angle + " deg, safety timeout " + timeout + " s")
})
```

## Manual control (EMG)

Press **Start**: the system switches the aircraft to EMG and deflects the control
surfaces one by one in random order. The technician names the deflection they see,
the operator presses the matching button. A wrong answer fails the step.

| Surface | Deflections |
|---|---|
| Ailerons | roll left, roll right |
| Elevator | nose up, nose down |
| Rudder | left, right |

All six deflections must be confirmed. Controls are released when the step ends.
**Restart** starts the sequence over with a new random order.

```js
var moves = shuffle([
    { name: "Roll left", roll: -1 }, { name: "Roll right", roll: 1 },
    { name: "Nose up", pitch: -1 }, { name: "Nose down", pitch: 1 },
    { name: "Rudder left", yaw: -1 }, { name: "Rudder right", yaw: 1 },
])
var names = moves.map(function (m) { return m.name }).sort()
function release() { cmd.rc.roll = 0; cmd.rc.pitch = 0; cmd.rc.yaw = 0 }
step.cleanup(release)
function deflect(i) {
    if (i >= moves.length) {
        release()
        return step.pass("all six deflections confirmed")
    }
    var m = moves[i]
    release()
    cmd.rc.roll = m.roll || 0
    cmd.rc.pitch = m.pitch || 0
    cmd.rc.yaw = m.yaw || 0
    step.info("deflection " + (i + 1) + " of " + moves.length + ": ask the technician what moved")
    step.ask(names.concat([{ text: "Restart", kind: "warn", restart: true }]), function (answer) {
        if (answer !== m.name) return step.fail("expected " + m.name + ", technician saw " + answer)
        deflect(i + 1)
    })
}
step.ask([{ text: "Start", kind: "primary" }], function () {
    cmd.proc.mode = proc_mode_EMG
    cmd.rc.mode = rc_mode_manual
    deflect(0)
})
```

## Semi-automatic control (RPV)

Put the aircraft level and press **Start**: the system switches to RPV and waits for
roll and pitch within {{cfg.levelMaxDeg}} deg. Then the operator tells the technician how to tilt
the aircraft, the system detects the tilt ({{cfg.tiltMinDeg}}..{{cfg.tiltMaxDeg}} deg) and asks whether
the control surfaces reacted correctly.

| Tilt | Expected reaction |
|---|---|
| Roll left, roll right | ailerons counteract the roll |
| Nose down, nose up | elevator counteracts the pitch |

When all tilts are confirmed the system switches the aircraft to TAXI.

```js
var tilts = [
    { name: "Tilt left", axis: "roll", sign: -1 },
    { name: "Tilt right", axis: "roll", sign: 1 },
    { name: "Nose down", axis: "pitch", sign: -1 },
    { name: "Nose up", axis: "pitch", sign: 1 },
]
function angle(axis) { return est.att[axis] * deg }
function level() { return Math.abs(angle("roll")) < cfg.levelMaxDeg && Math.abs(angle("pitch")) < cfg.levelMaxDeg }
function tilt(i) {
    if (i >= tilts.length) {
        cmd.proc.mode = proc_mode_TAXI
        return step.pass("all tilts confirmed, aircraft in TAXI")
    }
    var t = tilts[i]
    step.info(t.name + ": tell the technician, waiting for " + cfg.tiltMinDeg + ".." + cfg.tiltMaxDeg + " deg")
    step.poll(function () {
        var a = angle(t.axis) * t.sign
        step.info(t.name + ": " + t.axis + " " + angle(t.axis).toFixed(0) + " deg")
        return a >= cfg.tiltMinDeg && a <= cfg.tiltMaxDeg
    }, 60, function () {
        step.info(t.name + " detected, do the control surfaces counteract?")
        step.ask(["Correct", { text: "Wrong", kind: "danger" }], function (answer) {
            if (answer !== "Correct") return step.fail(t.name + ": control surfaces reaction is wrong")
            tilt(i + 1)
        })
    })
}
step.ask([{ text: "Start", kind: "primary" }], function () {
    cmd.proc.mode = proc_mode_RPV
    cmd.rc.mode = rc_mode_auto
    step.info("waiting for the aircraft to be level")
    step.poll(level, 60, function () { tilt(0) })
})
```

## Magnetometer

The operator tells the technician to turn the aircraft to the next direction and
presses **Check**. The estimated heading must match the direction within
{{cfg.magToleranceDeg}} deg.

Directions: north, west, south, east (counter-clockwise), from the start the aircraft
makes a full turn.

```js
var dirs = [{ name: "north", hdg: 0 }, { name: "west", hdg: 270 }, { name: "south", hdg: 180 }, { name: "east", hdg: 90 }]
function heading() { return (est.att.yaw * deg + 360) % 360 }
function check(i) {
    if (i >= dirs.length) return step.pass("all four directions within " + cfg.magToleranceDeg + " deg")
    var d = dirs[i]
    step.info("turn the aircraft to " + d.name + " (" + d.hdg + " deg)")
    step.ask([{ text: "Check", kind: "primary" }], function () {
        var h = heading()
        var err = Math.abs(bound(h - d.hdg))
        if (err > cfg.magToleranceDeg) return step.fail(d.name + ": heading " + h.toFixed(0) + " deg, error " + err.toFixed(0) + " deg")
        check(i + 1)
    })
}
check(0)
```

## Pitot

Put the cover on the pitot tube and press **Start**. The system checks that the
airspeed is below {{cfg.airspeedZeroMax}} m/s. Then remove the cover and blow into the tube:
the airspeed must rise above {{cfg.airspeedBlowMin}} m/s.

```js
step.ask([{ text: "Start", kind: "primary" }], function () {
    step.info("checking zero airspeed with the cover on")
    step.poll(function () {
        step.info("airspeed " + sns.pitot.airspeed.toFixed(1) + " m/s, must be below " + cfg.airspeedZeroMax)
        return sns.pitot.airspeed < cfg.airspeedZeroMax
    }, 10, function () {
        step.info("remove the cover and blow into the pitot tube")
        step.poll(function () {
            step.info("blow into the pitot: airspeed " + sns.pitot.airspeed.toFixed(1) + " m/s, must exceed " + cfg.airspeedBlowMin)
            return sns.pitot.airspeed > cfg.airspeedBlowMin
        }, 60, function () { step.pass("zero airspeed and pitot reaction OK") })
    })
})
```

## Parachute

Press **Start**: the system opens the parachute locks one after another
({{cfg.ers.length}} locks, Rel-1 first), the technician confirms every opening.
After the last lock the system closes all locks; confirm that the risers are back in
their locks, the auxiliary lines are removed and the cover is closed.

```js
function lockAll() { cfg.ers.forEach(function (p) { set(p, 0) }) }
step.cleanup(lockAll)
function open(i) {
    if (i >= cfg.ers.length) {
        lockAll()
        step.info("all locks closed")
        return step.ask(["Risers installed, lines removed, cover closed"], function () {
            step.pass("locks checked and restored")
        })
    }
    set(cfg.ers[i], 1)
    step.info("Rel-" + (i + 1) + " opened (" + cfg.ers[i] + ")")
    step.ask(["Rel-" + (i + 1) + " is open", { text: "Did not open", kind: "danger" }], function (answer) {
        if (answer === "Did not open") return step.fail("Rel-" + (i + 1) + " did not open")
        open(i + 1)
    })
}
step.ask([{ text: "Start", kind: "primary" }], function () { open(0) })
```

## Fuel pumps

Press **Start**: the system switches the {{cfg.pumps.length}} pumps on one by one,
the technician confirms each. Then the fuel transfer is switched to
automatic mode and the ignition is turned on to check the transfer algorithm.
Confirm the result, the ignition is turned off.

```js
function pumpsOff() { cfg.pumps.forEach(function (p) { set(p, 0) }) }
step.cleanup(function () { pumpsOff(); ctr.eng.ign1 = 0 })
function pump(i) {
    if (i >= cfg.pumps.length) {
        pumpsOff()
        ctr.eng.ign1 = 1
        step.info("automatic fuel transfer, ignition on")
        return step.ask(["Transfer OK", { text: "Transfer failed", kind: "danger" }], function (answer) {
            ctr.eng.ign1 = 0
            if (answer !== "Transfer OK") return step.fail("automatic fuel transfer failed")
            step.pass("pumps and automatic transfer OK, ignition off")
        })
    }
    pumpsOff()
    set(cfg.pumps[i], 1)
    step.info("pump " + (i + 1) + " on (" + cfg.pumps[i] + ")")
    step.ask(["Pump " + (i + 1) + " is running", { text: "Not running", kind: "danger" }], function (answer) {
        if (answer === "Not running") return step.fail("pump " + (i + 1) + " is not running")
        pump(i + 1)
    })
}
step.ask([{ text: "Start", kind: "primary" }], function () { pump(0) })
```

## Catapult

Put the aircraft 2-3 m behind the catapult pointing along the launch direction and
press **Aircraft positioned**: the system records the heading. Install the aircraft
on the catapult without turning or tilting it, then press **Installed**. If the heading
drifts by more than {{cfg.catapultHeadingToleranceDeg}} deg the step fails and the positioning
is repeated. Engine start stays blocked until the Takeoff mode is activated.

```js
function heading() { return (est.att.yaw * deg + 360) % 360 }
step.ask([{ text: "Aircraft positioned", kind: "primary" }], function () {
    var h0 = heading()
    step.info("heading " + h0.toFixed(0) + " deg recorded, install the aircraft on the catapult")
    step.poll(function () { return Math.abs(bound(heading() - h0)) > cfg.catapultHeadingToleranceDeg }, 0, function () {
        step.fail("heading changed by more than " + cfg.catapultHeadingToleranceDeg + " deg, repeat the positioning")
    })
    step.ask(["Installed"], function () { step.pass("installed on the catapult, heading " + h0.toFixed(0) + " deg") })
})
```

## Takeoff preparation

1. Switch the aircraft to **Takeoff** mode.
2. Start the engine: the system watches the engine status and errors.
3. Press **next stage** when the engine runs without errors, the system checks for
   factors preventing the takeoff.
4. **Ready**: the engine must reach {{cfg.rpmReady}} rpm, no errors, nothing prevents the takeoff.

On failure the engine rpm is reduced to zero and the takeoff is prohibited.

```js
function engineOk() { return val(cfg.engineStatus.path) == cfg.engineStatus.ok }
var ready = false
step.cleanup(function () { if (!ready) cmd.rc.thr = 0 })
step.info("switch the aircraft to TAKEOFF mode")
step.poll(function () { return cmd.proc.mode == proc_mode_TAKEOFF }, 120, function () {
    step.info("start the engine")
    step.poll(function () {
        step.info("engine rpm " + sns.eng.rpm.toFixed(0) + ", status " + (engineOk() ? "OK" : "not OK"))
        return sns.eng.rpm > 0 && engineOk()
    }, 120, function () {
        step.ask([{ text: "next stage", kind: "primary" }], function () {
            cmd.proc.action = proc_action_next
            step.info("waiting for the engine to reach " + cfg.rpmReady + " rpm")
            step.poll(function () {
                step.info("engine rpm " + sns.eng.rpm.toFixed(0) + " of " + cfg.rpmReady)
                return sns.eng.rpm >= cfg.rpmReady && engineOk()
            }, 60, function () {
                ready = true
                step.pass("engine at " + sns.eng.rpm.toFixed(0) + " rpm, takeoff allowed")
            })
        })
    })
})
```
