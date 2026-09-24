---
page: plugins
---

# Sun position

Shows the direction to the sun from the current unit on the map: a dashed ray from the unit icon towards the sun azimuth, the sun icon and the `AZ`/`EL` label. The color depends on the sun elevation: yellow - daylight, orange - low sun (below 15°, glare risk), gray - the sun is below the horizon.

The sun position is calculated by the same method as used by [suncalc.org](https://www.suncalc.org) (J. Meeus, *Astronomical Algorithms*): apparent solar coordinates with delta T, nutation, parallax and atmospheric refraction. For the sun above the horizon the results match suncalc.org within 0.002°.

The plugin menu (`Tools` → `Sun position`) shows:

- **Azimuth** - direction to the sun from true North;
- **Elevation** - sun angle above the horizon (refraction corrected);
- **Relative bearing** - direction to the sun from the aircraft nose, left or right;
- **Relative elevation** - sun angle above or below the aircraft wings plane, based on the current attitude;
- **Time** - UTC time used for calculation and its source.

Settings:

- **Show on map** - show or hide the sun direction on the map;
- **Time source** - `Auto` uses GPS time from telemetry (`est.sys.time`) when it is valid and the system clock otherwise, so the sun position is correct when replaying telemetry records. `System clock` always uses the computer time.

**Open suncalc.org** opens the website with the current unit position and time. The website expects local time of the location, the time zone of the GCS computer is used.
