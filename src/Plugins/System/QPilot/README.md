# QPilot

QPilot is a read-only telemetry MCP bridge plugin for APX Ground Control.

The plugin starts a local MCP HTTP endpoint:

`POST http://127.0.0.1:9876/mcp`

The MCP tool is:

* `gcs_telemetry_json`

## Read-only mode

QPilot never writes Mandala facts, never executes control commands, never uploads missions, and never controls vehicles.

It reads APX/GCS telemetry values through the local application context and returns compact JSON.

## Telemetry output

QPilot tracks a fixed startup list of telemetry facts every 200 ms.

The telemetry response contains:

* current value;
* total online statistics;
* 30-second rolling window statistics;
* correlations for requested numeric facts.

Raw flight history is not returned.
