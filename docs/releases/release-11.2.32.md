# New Features
* RSSI window and explicit mode per GCS unit
* offline map tiles download
* script controls in editor toolbar
* Highlight mission paths crossing deny geofences
* Visualize Smooth Landing Approach to TA Boundary
* add EMIP engine monitoring instrument plugin
* add lite layout profile for simplified ground control
* hide application updates in lite profile
* lite layout override file

# Bug Fixes
* nodes exchange between GCS
* wait for GCS nodes
* only top GCS downloads nodes
* nodes exchange at protocol level
* nodes exchange on search request
* accept search request from GCS
* keep node download on repeated ident
* elevation map corrections
* limit uncompressed size of GCS nodes data
* save node conf only after dict saved
* resume nodes download after failure
* geo lines bug
* nodes resume and multi-GCS sync
* telemetry recording overlap (closes [`114`](https://github.com/uavos/apx-gcs/issues/114))
* Numbers close menu page on remove [`81`](https://github.com/uavos/apx-gcs/issues/81)
* LOS warning
* plot discrete fields as steps
* Geofence fill opacity
* map vignette shader on Linux
* quazip deprecated QDateTime
* mission active item follows index
* keep user EMIP config intact on parse errors
* native menu bar follows fact visibility and model changes
* restore closing brace of parseConfData lost in merge
* apply serial baud rate before open and fail on error
* track serial port path before open so closePort releases it
* apply serial baud rate change to open port
* default serial baud rate when missing or invalid in config
* copilot review findings
* handle invalid serial baud rate change on open port
* reorder and clean up recommendations in extensions.json
* copilot review findings for serial baud handling

# Refactoring
* register the FP controls page from the EMIP plugin
* hide layout items by name

# Performance Enhancements
* Improve KML Loading Performance and Smooth Map Rendering

# Administration and Chores
* remove EMIP plugin, moved to its own branch
* drop plugin page registration, it belongs to the EMIP plugin

# Comments

**fix: geo lines bug**

Mission fix line geofences loading

**fix: telemetry recording overlap**

telemetry: start new record on stream timestamp rollback

**feat: offline map tiles download**

Map download visible area tiles to cache

**feat: script controls in editor toolbar**

Nodes: script control commands in script editor

**fix: map vignette shader on Linux**

Fix map vignette shader on Linux
