---
page: plugins
---

# Traffic

Map view plugin: ADS-B traffic and airspaces around the current unit (or the map center when the unit has no position).

- **ADS-B**: [adsb.fi](https://adsb.fi) open data, `point` request every second, no key. On HTTP 429 the requests back off up to 2 min. Aircraft are drawn on the map (icon by track, callsign, altitude, speed) and listed in the Traffic instruments panel (button next to the Terminal), nearest first. Colors: red - closer than 10 km and 600 m by altitude, yellow - closer than 25 km, cyan - other.
- **Airspaces**: CTR, TMA, R, D, P ... from the [OpenAIP](https://www.openaip.net) country export (`<cc>_asp.geojson`, no key) as outlines with name and limits. The country file is downloaded every 10 min and parsed in a worker thread, the map shows the airspaces within 1.5 radius of the origin and follows the origin as it moves.

Tools menu: `use` turns the plugin on and off and brings the panel to front, `radius` in km, `country` of the airspaces (`-` is off). Request errors are shown in the panel header as `ADS-B: HTTP 429`.
