# apx-fdm: flight dynamics for SIL without X-Plane

`apx-fdm` is a small command line program that replaces X-Plane as the
source of sensor data for the `sim` SIL node (`sim-darwin`, `sim-linux`).
It runs a rigid body 6-DOF fixed-wing model for `N` aircraft in one
process and talks to `N` SIL node instances over UDP with the same
protocol as the X-Plane plugin in [`../xplane`](../xplane/plugin.cpp).
There is no graphics: the GCS map is the visualisation.

## Build and run

The target is part of the GCS build (`ninja -C build apx-fdm`), the
binary is `build/out/bin/apx-fdm` and it is installed next to the `gcs`
executable. It depends only on the shared `xbus`, `xbus.tcp` and
`mandala` modules (no Qt).

```
apx-fdm [options]
  -n, --count N       number of aircraft / SIL instances (1..10)
                      [auto: serve every SIL node that answers on its ports]
      --lat DEG       start point latitude
      --lon DEG       start point longitude
      --alt M         start point (ground) altitude MSL
      --hdg DEG       start heading
      --spacing M     distance between aircraft along the heading [50]
      --speedup X     simulation time multiplier [1.0]
      --host ADDR     SIL node host to send sensors to [127.0.0.1]
      --rate HZ       sensors output rate [100]
      --wind-spd MPS  wind speed [0]
      --wind-dir DEG  wind direction (from) [0]
      --model FILE    aircraft model file [built-in]
      --model-dump    print the model and exit
  -v, --verbose       print state of every aircraft once a second
```

Without `--count` the program opens the control ports of all 10 slots
and runs the model of slot `i` only while the SIL node on ports
`+100*i` answers the config request: nodes can be started and stopped
at any time, a node that goes away while its aircraft stands on the
ground gets the aircraft reset to the start point. `--count N` limits
the slots to `0..N-1`.

Aircraft `#i` is placed `i * spacing` metres ahead of the start point
along the start heading, standing on its wheels on flat ground at the
start altitude. The start point should be the runway threshold of the
mission (`shiva.takeoff.check.hdg/xtrack` verify heading and cross
track before takeoff).

Typical session with one SIL node:

```sh
cd ~/Documents/UAVOS/Firmware/sim && ./sim-darwin &
apx-fdm --lat 46.9760158 --lon -122.9024902 --alt 63 --hdg 196 -v
```

With `N` nodes, instance `i` must use the ports `uplink 9225+100*i`,
`sensors 9233+100*i`, `controls 9231+100*i` (`sim-darwin -u -s -x`),
which is what the GCS Simulator plugin does for its `SIL instances`
option; `apx-fdm` uses the same port stride and picks the nodes up as they start.

`--speedup` scales simulated time against the wall clock; the SIL node
itself runs in real time, so values above ~2 make the autopilot see a
faster world. If the process cannot keep up it skips ahead and prints a
warning.

## Protocol

Everything is UDP, every datagram is one xbus packet (`xbus::pid_s` +
payload) framed with COBS (`serial/CobsEncoder.h`, `CobsDecoder.h`).
For aircraft `i`:

- `apx-fdm` binds `UDP_PORT_SIM_CTR + 100*i` (9231) and receives
  controls there; it sends sensors to `host:UDP_PORT_AP_SNS + 100*i`
  (9233). Constants are in `shared/xbus/tcp/tcp_ports.h`.
- `cmd.env.sim.cfg` / `pri_request` (empty payload) is sent every 2 s
  until the node answers with `pri_response`: a list of NUL terminated
  strings, one per bound control channel. The strings are the X-Plane
  dataref names from the node config (`controls.mixer.ch_xpl`). The
  node also re-sends the list on its own when its config changes.
- `cmd.env.sim.ctr` / `pri_final` arrives at 100 Hz with one `float`
  per configured channel, in the same order as the names. If no
  controls arrive for 2 s the channel assignment is dropped and the
  config is requested again (node restart).
- `cmd.env.sim.sns` / `pri_final` carries `mandala::bundle::sim_s`
  (`shared/mandala/MandalaBundles.h`) at `--rate` Hz (default 100), sent
  only while the channel assignment is known:

| field | content |
|---|---|
| `att_deg[3]` | roll, pitch, heading [deg], heading 0..360 |
| `gyro_deg[3]` | body angular rates [deg/s] |
| `acc_ned[3]` | specific force in NED [m/s^2]; (0, 0, -9.81) at rest |
| `lat_deg`, `lon_deg`, `hmsl` | position; flat earth around the start point |
| `vel_ned[3]` | ground velocity NED [m/s] |
| `agl` | CG height above the (flat) ground [m] |
| `rpm` | engine speed |
| `airspeed` | equivalent airspeed [m/s] (the node converts it to pitot pressure with sea level density) |
| `baro_mbar` | ISA static pressure at `hmsl` [hPa] |
| `air_temp`, `room_temp` | ISA temperature at `hmsl` [C] and +20 C |
| `slip` | sideslip angle [deg] |

Sensor noise and biases are added by the node (`sim.*` config), the
simulator sends clean truth. The `cmd.env.sim.usr` user data channel of
the X-Plane plugin is not implemented.

### Control channel names

The dataref names are treated as names of model inputs. Both the full
X-Plane path and its last component are accepted and the `[n]` array
index is ignored, so the stock unit configs
(`apx-fdm-xpl/conf/*.unit`) work unchanged:

| dataref | input | range |
|---|---|---|
| `sim/joystick/yoke_roll_ratio` | aileron (+ roll right) | -1..1 |
| `sim/joystick/yoke_pitch_ratio` | elevator (+ nose up) | -1..1 |
| `sim/joystick/yoke_heading_ratio` | rudder and nose wheel steering (+ nose right) | -1..1 |
| `sim/flightmodel/controls/rud_trim` | rudder only, added to the above | -1..1 |
| `sim/flightmodel/engine/ENGN_thro_use[0]` | throttle | 0..1 |
| `sim/flightmodel/controls/flaprqst` | flaps | 0..1 |
| `sim/flightmodel/controls/parkbrake` | wheel brakes (main gear) | 0..1 |
| `sim/flightmodel/controls/sbrkrqst` | airbrake / spoilers | 0..1 |

A few X-Plane aliases are recognised too (`throttle_ratio`,
`flap_ratio`, `parking_brake_ratio`, `speedbrake_ratio`, ...). Unknown
names are reported in the log once and ignored.

## Model

`Aircraft.cpp` follows the structure of the PX4 SIH simulator
(`sih.cpp`, BSD-3-Clause): per step, generate forces and moments in the
body frame, then integrate the rigid body equations of motion with a
fixed step (500 Hz, semi-implicit Euler, quaternion attitude). The
aerodynamics use a conventional set of linear stability and control
derivatives instead of the SIH flat plate segments:

- lift `CL = CL0 + CLa*alpha + CLq*q^ + CLde*de + CLdf*df`, blended
  with a flat plate model beyond the stall angle (`alpha_stall`,
  `stall_width`), flaps lower the stall angle by 2 deg;
- drag `CD = CD0 + CL^2/(pi*e*AR) + CDdf*df + CDsb*dsb + CDb*beta^2`;
- side force and roll/pitch/yaw moments from `beta`, the
  nondimensional rates `p^ q^ r^` and the surface commands
  (`CYb, Clb, Clp, Clr, Clda, Cm0, Cma, Cmq, Cmde, Cnb, Cnr, Cndr`, ...);
- propeller thrust `T = thr * P / (V + P/T_static)`: static thrust at
  low speed, power limited at high speed; engine speed follows the
  throttle with a first order lag;
- landing gear: three wheels, each a vertical spring-damper with
  saturated viscous friction (rolling resistance, brakes on the main
  wheels, lateral tyre friction); the nose wheel follows the steering
  command with authority fading with ground speed;
- flat earth: the ground is a plane at `--alt`, lat/lon are computed on
  a local tangent plane (WGS84 radii at the start latitude);
- ISA atmosphere for density, pressure and temperature.

The default parameters (`AircraftParams` in `Aircraft.h`) describe a
~1200 kg fixed-wing UAV close to the `S1C` X-Plane model and to the
`SIM-S1C.unit` autopilot config: wing 14 m^2 / 16 m span, clean stall
at ~29 m/s, 150 kW effective power, rotation at 32 m/s after ~280 m of
runway, ~5 m/s climb at 42 m/s, 40 m/s cruise at ~27% throttle. They
are compile time constants for now.

Limitations: no ground effect, no engine torque or propeller
slipstream, no terrain (flat ground everywhere), wind is constant.

## Verified

Checked on 2026-10-23 with `sim-darwin` 11.2.24 and the `SIM-S1C.unit`
config: the node gets a 3D GPS fix at the start point, INS initialises,
`TAKEOFF` + `next` releases the brakes, runs to 32 m/s, lifts off,
climbs and switches to `WPT`, then flies the mission at 42 m/s holding
altitude. Three aircraft with three SIL instances (ports +100/+200) run
stably; `apx-fdm` needs ~0.5% of a core per aircraft (3 aircraft, one
of them flying: 1.6..2%), each `sim-darwin` ~3.3%.

## Aircraft model

Without `--model` the built-in model is used: a ~1200 kg MALE class UAV. Any
other aircraft is described by a text file of `name value` pairs, where the
names are the `AircraftParams` fields of [`Aircraft.h`](Aircraft.h), angles are
in degrees and everything else is in SI units:

```
mass               25
inertia            5.0 3.0 7.5      # Ixx Iyy Izz
S                  0.9
alpha_stall        14               # deg
wheel              0.5 0 0.25  8e3 600  1 0   # x y z  k c  steerable brakes
```

Values that are not listed keep their built-in defaults, so a file only has to
describe what differs. `#` starts a comment, unknown names are reported and
skipped, a malformed value stops the app. The three `wheel` lines are the nose
and the two main wheels, in that order.

Use `--model-dump` to get the complete current model in the same format, which
is the easiest way to start a new one:

```sh
apx-fdm --model-dump > my-aircraft.fdm
apx-fdm --model my-aircraft.fdm
```

The models shipped with the GCS are in `resources/fdm`, they are listed by the
`Model` option of the simulator plugin.
