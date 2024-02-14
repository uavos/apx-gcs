#include "TelemetryVariables.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryReader.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

namespace RG {

TelemetryVariables::TelemetryVariables()
{
    auto reader = Vehicles::instance()->f_replay->f_telemetry->f_reader;

    connect(reader, &TelemetryReader::dataAvailable, this, &TelemetryVariables::clearCacheSlot);
}

std::optional<FunctionResult> TelemetryVariables::call(QString name)
{
    auto it = m_registry.find(name);

    if (it == m_registry.end())
        return std::nullopt;

    return (*it.value())();
}

void TelemetryVariables::clearCache()
{
    for (auto el : m_registry) {
        el->clearCache();
    }
}

void TelemetryVariables::clearCacheSlot()
{
    clearCache();
}

} // namespace RG
