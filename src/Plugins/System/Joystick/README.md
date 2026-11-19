---
page: plugins
---

# Joystick interface

The plugin makes joystick controls (axes, buttons, hats) available to the UAV control logic.

Devices are accessed through operating system services, without external libraries:

- Linux: kernel `evdev` interface (`/dev/input/event*`);
- macOS: IOKit HID Manager.
