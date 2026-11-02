---
page: plugins
---

# Traffic

Map view plugin: ADS-B traffic and airspaces around the current unit (or the map center when the unit has no position).

- **ADS-B**: [adsb.fi](https://adsb.fi) open data, `point` request every second, no key. On HTTP 429 the requests back off up to 2 min. Aircraft are drawn on the map as icons by the ADS-B emitter category (airplane, helicopter, balloon, parachutist, UAV, ground vehicle, obstacle) rotated by track. Hovering an icon shows the info panel (callsign, category and type, altitude, vertical speed, speed, distance and bearing), a click pins it to the aircraft (click again to unpin). All altitudes are in metres: `GNSS` is the geometric altitude above the WGS84 ellipsoid when the aircraft reports it, `BARO` is the pressure altitude otherwise; the panel shows both when available and labels which one is used. Aircraft not updated for 30 s (request errors, backoff) are removed from the map. Colors are by the distance from the origin only (the unit, or the map center when the unit has no position), altitudes are shown but not involved: red - closer than 2 km, yellow - closer than 4 km, cyan - other. The Traffic instruments panel (button next to the Terminal) lists the aircraft nearest first; a click centers the map on the aircraft and pins its info panel.
- **Airspaces**: CTR, TMA, R, D, P ... from the [OpenAIP](https://www.openaip.net) country export (`<cc>_asp.geojson`, no key) as outlines with name and limits. The country file is downloaded every 10 min and parsed in a worker thread, the map shows the airspaces within 1.5 radius of the origin and follows the origin as it moves.

Tools menu: `use` turns the plugin on and off and brings the panel to front, `radius` in km, `country` of the airspaces (`-` is off). While the plugin is off its panel and button are not in the instruments row. Request errors are shown in the panel header and in the menu item description as `ADS-B: HTTP 429`.

**Offline**: when the OS reports the network unreachable or a request fails below HTTP (no route, DNS, timeout) the panel shows `OFFLINE` instead of the list and the menu item description reads `OFFLINE`. ADS-B keeps probing every 5 s (5 s timeout), the first reply clears the state. The airspaces download is tried once: without a connection the `country` item reads `Offline, restart the plugin to retry` and is locked until `use` is switched off and on.
