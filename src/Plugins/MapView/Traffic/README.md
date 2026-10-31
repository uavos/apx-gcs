---
page: plugins
---

# Traffic

Map view plugin: ADS-B traffic and airspaces around the current unit (or the map center when the unit has no position).

- **ADS-B**: [adsb.fi](https://adsb.fi) open data, `point` request every second, no key. On HTTP 429 the requests back off up to 2 min. Aircraft are drawn on the map as icons by the ADS-B emitter category (airplane, helicopter, balloon, parachutist, UAV, ground vehicle, obstacle) rotated by track. Hovering an icon shows the info panel (callsign, category and type, altitude, vertical speed, speed, distance and bearing), a click pins it to the aircraft (click again to unpin). All altitudes are in metres: `GNSS` is the geometric altitude above the WGS84 ellipsoid when the aircraft reports it, `BARO` is the pressure altitude otherwise; the panel shows both when available and labels which one is used. Colors: red - closer than 10 km and 600 m by altitude, yellow - closer than 25 km, cyan - other.
- **Airspaces**: CTR, TMA, R, D, P ... from the [OpenAIP](https://www.openaip.net) country export (`<cc>_asp.geojson`, no key) as outlines with name and limits. The country file is downloaded every 10 min and parsed in a worker thread, the map shows the airspaces within 1.5 radius of the origin and follows the origin as it moves.

Tools menu: `use` turns the plugin on and off, `radius` in km, `country` of the airspaces (`-` is off). Request errors are shown in the menu item description as `ADS-B: HTTP 429`.
