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

