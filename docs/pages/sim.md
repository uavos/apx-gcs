---
---

# Simulation

To get started with **Software in the loop simulation** (no hardware) you can follow the following instructions:

- [Install Ground Control](install) application;
- Install [X-plane Flight Simulator](http://www.x-plane.com/);
  - The system works well with **X-plane 10**;
- Launch the X-plane Simulator for the first time to ensure it is working and configured;
  - Configure minimal graphics settings and disable all sounds;
  - Exit the simulator app through menu;
- Launch the GCS application;
  - Open dialog through menu `Tools/Simulator`;
    - Select your X-Plane installation from the drop-down box;
    - Turn on the switch to enable X-plane launch;
    - Press `Launch` button;
    - Wait until X-plane window loads;
- Switch to X-Plane window;
  - Select the airport `KSEA`;
  - Select runway `RWY - 1 6 L`;
  - Choose aircraft `UAVOS/MALE-B`
- Switch to GCS window, wait until `SIM` vehicle is recognized, select it if needed in the `Vehicle` menu;
  - Open the `Vehicle parameters` window and make sure you have updated and uploaded configuration;
  - When there is no mission downloaded from vehicle exists - i.e. mission label on the map does not show any mission:
    - Press mission button to open dialog, then `Load mission` button, to load applicable mission, then click `Upload` button to sync mission with the vehicle;
    - Alternatively, create a new mission and upload to the vehicle;
- If everything is ok, you will see the telemetry data and it should say that GPS is available and show to you the current position (from simulation) on the map;
- Use the commands widget in the GCS main window to control the UAV:
  - Press `TAKEOFF` button to start takeoff procedure;
  - Press `NEXT` button for the next stage of the procedure;
- The UAV should perform takeoff and fly waypoints, then land automatically;

## Notes

- X-plane and [simulated autopilot](/hw/sim) exchange data via x-plane [plugin](https://github.com/uavos/apx-gcs/tree/main/src/Plugins/System/Simulator/xplane) and TCP/IP network. The plugin binary and aircraft models resources are installed automatically by the GCS when you launch the simulation from the GCS menu.

## Telemetry sample replay

There is default telemetry record exist in the database and it could be replayed in the freshly installed GCS. To do this, follow these steps:

- Launch the GCS application;
- Select `REPLAY` vehicle;
- Go to vehicle menu, then open `Telemetry` entry;
  - Load telemetry record in `Records` entry;
  - Open `Player` sub-menu and press `Play` button. The GCS should replay the recorded telemetry;
- Open window `Windows/Telemetry` (app menu) for telemetry data chart;

## Reset to defaults

To start everything from scratch - remove or rename the `~/Documents/UAVOS` folder.
