#include "TelemetryFunctions.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

namespace ReportGenerator {

TelemetryFunctions::TelemetryFunctions()
{
    auto telemetry = Vehicles::instance()->f_replay->f_telemetry;
    auto reader = telemetry->f_reader;

    connect(reader, &TelemetryReader::dataAvailable, this, &TelemetryFunctions::clearCacheSlot);
}

std::optional<QVariant> TelemetryFunctions::call(QString name)
{
    auto it = m_registry.find(name);

    if (it == m_registry.end() || it.value() == nullptr)
        return std::nullopt;

    return (*it.value())();
}

void TelemetryFunctions::clearCache()
{
    for (auto el : m_registry) {
        el->clearCache();
    }
}

void TelemetryFunctions::clearCacheSlot()
{
    clearCache();
}

TelemetryFunc::TelemetryFunc(TelemetryFunctions *parent,
                             QString func_name,
                             std::function<QVariant()> func)
    : ParamlessCachedFunction(func)
{
    parent->m_registry[func_name] = this;
}

} // namespace ReportGenerator
