#include "TelemetryFunctions.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

TelemetryFuncRegistry::TelemetryFuncRegistry()
{
    auto telemetry = Vehicles::instance()->f_replay->f_telemetry;
    auto reader = telemetry->f_reader;

    connect(reader,
            &TelemetryReader::dataAvailable,
            this,
            &TelemetryFuncRegistry::telemetry_data_changed);
}

TelemetryFuncRegistry &TelemetryFuncRegistry::instance()
{
    static TelemetryFuncRegistry inst;
    return inst;
}

std::optional<QVariant> TelemetryFuncRegistry::call_by_name(QString name)
{
    auto it = m_registry.find(name);

    if (it == m_registry.end() || it.value() == nullptr)
        return std::nullopt;

    return (*it.value())();
}

void TelemetryFuncRegistry::clear_cache()
{
    for (auto el : m_registry) {
        el->clear_cache();
    }
}

void TelemetryFuncRegistry::telemetry_data_changed()
{
    clear_cache();
}

TelemetryFunc::TelemetryFunc(TelemetryFuncRegistry *parent,
                             QString func_name,
                             std::function<QVariant()> func)
    : CachedFunction(func)
{
    parent->m_registry[func_name] = this;
}