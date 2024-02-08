#include "TelemetryResolver.h"
#include "TelemetryFunctions.h"

TelemetryResolver::TelemetryResolver()
    : m_functions(TelemetryFuncRegistry::instance())
{}

std::optional<QString> TelemetryResolver::get_value(QString field_name)
{
    auto temp = m_functions.call_by_name(field_name);
    if (!temp.has_value())
        return std::nullopt;
    return temp.value().toString();
}
