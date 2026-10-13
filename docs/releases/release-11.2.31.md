# New Features
* select unit to track
* map overlay with antenna beam and elevation

# Bug Fixes
* map hover blocked by the elevation layer
* map ruler (hover blocked by the elevation layer)
* udp port receives other multicast groups on the same port
* udp port receives other multicast groups on the same port
* Telemetry load first nodes record

# Comments

**fix: udp port receives other multicast groups on the same port**

DatalinkSocketUdp: drop datagrams addressed to a multicast group other
than the one from the bind= query. A socket bound to 0.0.0.0:<port>
receives datagrams of every group the host has joined on that port, so
two UDP ports with different groups and the same port number each
received both units' telemetry. The datalink forwarded it to the other
port, which sent the unit its own downlink back and the unit reported
a squawk conflict. The removed udp_mcast remote serial port had the same
destination group check.

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
