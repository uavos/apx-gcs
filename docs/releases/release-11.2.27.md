# New Features
* Extended Signals plugin
* EKF status monitor
* add waypoint actions (shot, dshot, tshot))
* reverse waypoints order
* inherit xtrack from previous waypoint
* BLE Datalink Port connection
* Mission Reverse WP order tool
* plugin to display EKF status

# Bug Fixes
* fact shadow value
* extMode save
* console warnings overflow segfault
* node conf text fields format conversions
* datalink udp sockets config
* allow zero node field text
* json numbers conversion for '000' strings
* COBS overhead estimation

# Performance Enhancements
* update qt to 6.11.1 for macos CI
* improve serial port datalink options
* Ubuntu 26.04 LTS upgrade

# Comments

**feat: add waypoint actions**

Merge pull request [`115`](https://github.com/uavos/apx-gcs/issues/115) from uavos/mission

**feat: Mission Reverse WP order tool**

Merge pull request [`117`](https://github.com/uavos/apx-gcs/issues/117) from uavos:mission-edit-tools

**feat: plugin to display EKF status**

Merge pull request [`118`](https://github.com/uavos/apx-gcs/issues/118) from uavos/ekf-plugin
