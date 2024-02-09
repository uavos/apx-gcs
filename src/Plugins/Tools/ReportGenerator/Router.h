#pragma once

#include "rgTelemetry/TelemetryResolver.h"
#include <optional>
#include <QtCore>

/**
 * @brief The router is used to distribute requests between resolvers
 * 
 */
class Router
{
public:
    Router();

    std::optional<QString> resolve_path(QString command);

private:
    TelemetryResolver m_telemetry_resolver;
};
