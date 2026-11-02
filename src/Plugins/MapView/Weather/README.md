---
page: plugins
---

# Weather

Map view plugin: wind along the mission route of the current unit at the planned altitude, for the navigation computations.

- **Route**: the mission waypoints of the current unit with the start point (runway) when set, as the MissionPlanner draws them. The route is sampled every 100 m, plus a point on every waypoint. The altitude on a point is the altitude of the waypoint the leg goes to, as the autopilot commands it at the leg start; with `Altitude tracking` on the waypoint the altitude changes linearly along the leg. AGL altitudes are relative to the unit home altitude (`est.nav.ref.hmsl`), or to the forecast model terrain at the start when the unit has no home yet.
- **Forecast**: [Open-Meteo](https://open-meteo.com) hourly forecast, no key, one request for points every 10 km along the route (up to 60 points): wind at 10/80/120/180 m above the model terrain and at the 1000...500 hPa pressure levels with their geopotential heights. The forecast is refreshed every 30 min and when the route moves out of the loaded points; on a request error it is retried in a minute, the loaded forecast stays in use.
- **On every route point**: the wind is interpolated in height to the planned altitude and between the two nearest forecast points, at the current forecast hour.
- **Map**: an arrow on every point where the wind blows to, cyan on the route, yellow on the waypoints; thinned out when zoomed out, the waypoints are always shown. A dot instead of the arrow when there is no forecast. Hovering a point shows the info panel: waypoint and distance along the route, altitude MSL and AGL, wind direction (from) and speed with the forecast hour. A click pins the panel, click again (or on the panel) to unpin.

Tools menu: `use` turns the plugin on and off. The menu item description shows the route length, the wind range and the forecast time; request errors are shown as `Forecast: HTTP 429`.
