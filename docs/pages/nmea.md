# APX NMEA PLD Protocol

## NMEA packet format

### Definitions

- `NMEA` - short for National Marine Electronics Association, uses a simple format to transmit data between devices like GPS receivers and navigation systems.
- `GCS` - [Ground Control Software](https://docs.uavos.com/gcs/index).
- `AP` - Onboard Autopilot.
- `PLD` - Onboard Payload.
- `VCP` - [Virtual Comm Port](https://docs.uavos.com/fw/vcp).

### Structure

- `Start marker`: Each message begins with a dollar sign (`$`).
- `Talker ID`: a two-character code that identifies the device sending the message:
  - `AP` for Autopilot.
  - `GC` for Ground Control.
  - `PL` for Payload.
  - `AX` for auxiliary control device (a dedicated payload PC).
- `Message ID`: characters specifying the type of data in the message:
  - `D#` for data values. The `#` sign is a placeholder for dataset number. Data contains a comma separated list of field values (floating point numbers) defined in the Data Set of the corresponding number (`#`).
  - `S#` for serial data. Data contains a hexadecimal encoded data array. The sign `#` is a placeholder for serial port ID number.
- `Data Fields`: Comma-separated fields containing the actual data (e.g., coordinates, time, etc.). Empty fields indicate missing data. Can be just one field (f.ex. For serial data).
- `Checksum` (optional): An asterisk (`*`) followed by a two-character code used for error checking (not all messages have this).
- `Terminator`: The message ends with a carriage return (`CR`) and line feed (`LF`), or just with `LF`.
  
### Key points

- Messages are limited to 400 characters, including the start/end markers.
- Data is in ASCII format for readability.
- Checksums, when included, help ensure data integrity during transmission. Checksums can be omitted.

### Message examples

- `$GCD1,000000.000,1948.5555,1,10131.1111,2,1,08,0.5,45.0,3,4.8,5,0000,0000`
  
Telemetry data sent by GCS to the Auxiliary Payload PC. The list of values is defined in the data set number 1.

- `$APD2,1.2,3.4,0.1,0.4,99,25987.1`

Data values which are sent by AP to PLD. The list of data fields corresponds to data set number 2.

- `$PLS54,0A0BF3040506070809`

PLD sends this packet to AP to transmit arbitrary data to the ground. The VCP number 54 is transparently forwarded to GCS and available as a physical serial port on the GCS PC or via UDP. The mapping between VCP ID 54 and physical port is done by the GCS. I.e. The physical port on the ground will transmit 9 bytes (0A0BF3040506070809). Moreover, the GCS can forward the data to the auxiliary payload control computer as-is (NMEA), and it is configurable.

- `$APS54,01020304050607`

AP sends this packet to PLD to transmit arbitrary data to the payload which was received via physical serial port on the GCS PC or via UDP (received 7 bytes 01020304050607). The 54 is the VCP ID number and is configured on the GCS PC. Moreover, this message can be forwarded to PLD as-is, without any processing, and it is configurable.
