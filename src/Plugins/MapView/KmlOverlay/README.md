---
page: plugins
---

# KML overlay import

Imports Google Earth KML and KMZ (zipped KML) files to the map view. Used to display reference items only, i.e. geofences, airspace planning etc.

Zones are drawn in aeronautical chart style: a thin outline with a translucent band inside the boundary and no fill. When zoomed in, zones that are large enough on screen get a small faint label with the zone identifier and its upper and lower limits, when the file provides them (placemark `name` or `ExtendedData` fields).

All visible zones are rendered into a single texture in a background thread, so the map stays responsive regardless of the number of polygons.
