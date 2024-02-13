#include "TelemetryResolver.h"
#include "TelemetryFunctions.h"

namespace ReportGenerator {

TelemetryResolver::TelemetryResolver()
    : m_functions(TelemetryFunctions::getInstance())
{}

std::optional<QString> TelemetryResolver::getValue(QString command)
{
    auto temp = m_functions.call(command);
    if (!temp.has_value())
        return std::nullopt;
    return temp.value().toString();
}

}; // namespace ReportGenerator
