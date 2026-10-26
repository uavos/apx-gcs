---
page: plugins
---

# Simulator

Plugin searches for [X-Plane](https://www.x-plane.com) flight simulator installation and performs the following actions:

* copies bundled aircraft models from GCS resources;
* copies XPL plugin to the simulator from GCS bundle;
* launches X-Plane simulator application;
* launches SIL node simulation utility (provided by the firmware package);

## Multiple SIL instances

The `SIL instances` option launches several SIL node instances to simulate a fleet (f.ex. for swarm testing). Instance `#i` listens on UDP ports `uplink 9225+100*i`, `sensors 9233+100*i` and sends controls to `9231+100*i`. Instance `#0` uses the default ports (X-Plane plugin compatible) and the user `HOME`; every other instance gets its own `HOME` at `Documents/UAVOS/Firmware/sim/units/<i>` so that node configs are kept separately. Console output of each instance goes to `sim.log` in the same folder.

Instances after the first are started with `-i <i>`, which the SIL node appends to its callsign and
uses as the unit UID (`SIM1`, `SIM2`, ...). Without it every instance reports the same identity and
the GCS treats them as one unit. Instance `#0` is started without the option and keeps the identity
it has now.

## Simulator type

Besides the detected X-Plane installations, the `Type` option offers `apx-fdm`: the built-in flight
dynamics ([`fdm`](apx-fdm/README.md)), one process for all instances (up to `10`). It is launched by
this plugin with the start point taken from the first runway of the current mission (threshold, its
heading and `MSL`, the unit home reference when the runway altitude is `default`). X-Plane is not
needed and is not started. Stray `apx-fdm` apps are killed on start, as the SIL nodes are: one left
running from a manual session keeps the UDP ports and serves its own start point, so the aircraft
would appear somewhere else than the mission runway.

An X-Plane type uses the default ports, i.e. it serves one instance, so `SIL instances` is limited
to `1` for it.

