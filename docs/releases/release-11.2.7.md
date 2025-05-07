# New Features
* add functionality to disable local TCP ports on server bind to avoid loops

# Bug Fixes
* telemetry import for fresh gcs install
* config file parsers JSON object data handlers
* telemetry file load discard behavior
* telemetry plot cursor movement
* telemetry plot discrete data handling
* msg model pass through app evt queue
* loading of telemetry files with too many messages (freeze)
* telemetry db modified request to refresh records list (freeze)
* log views and console focus and auto scroll issues
* multi-gcs node params update behavior (closes [`91`](https://github.com/uavos/apx-gcs/issues/91))

# Performance Enhancements
* optimize containers iterators (remove unnecessary data copy)
