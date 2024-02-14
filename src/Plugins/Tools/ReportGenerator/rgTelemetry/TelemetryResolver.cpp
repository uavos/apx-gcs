#include "TelemetryResolver.h"
#include "TelemetryVariables.h"

namespace RG {

TelemetryResolver::TelemetryResolver()
    : m_variables(TelemetryVariables::getInstance())
{}

std::optional<QString> TelemetryResolver::getValue(QString command)
{
    auto temp = m_variables.call(command);
    if (!temp.has_value())
        return std::nullopt;
    auto ret_value = temp.value();
    return ret_value.value().toString();
}

}; // namespace RG
