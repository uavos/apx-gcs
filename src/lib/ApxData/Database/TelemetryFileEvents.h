#pragma once

#include "TelemetryFileWriter.h"

namespace telemetry {

using Event = TelemetryFileWriter::Event;

static const Event EVT_MSG{"msg", {"txt", "src"}};              // message event
static const Event EVT_CONF{"conf", {"param", "value", "uid"}}; // conf event

} // namespace telemetry
