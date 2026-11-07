# Aircraft check

Pre-flight check of the aircraft. Press **Start** and follow the steps:
automatic steps are evaluated by the system, the others wait for the operator.

### Before you begin

- the aircraft is powered and the datalink is up;
- the technician is at the aircraft.

## Battery voltage
type: auto
```js check
var v = mandala.sns.pwr.vsys.value;
({ ok: v >= cfg.batteryMin, msg: "vsys " + v.toFixed(1) + " V, min " + cfg.batteryMin + " V" })
```
System bus voltage is read from `sns.pwr.vsys` and compared with `batteryMin` from `checklist.json`.

If the voltage is below the minimum the step fails and the check continues with the next step.
