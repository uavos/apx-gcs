#pragma once

#include "TelemetryFileWriter.h"

namespace telemetry {

using Event = TelemetryFileWriter::Event;

static const Event EVT_MSG{"msg", {"uid", "src", "txt"}};       // message event
static const Event EVT_CONF{"conf", {"uid", "param", "value"}}; // conf event

} // namespace telemetry
