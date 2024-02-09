#pragma once

#include "rgInterfaces/IResolver.h"
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
    TelemetryResolver m_impl_telemetry_resolver;
    QMap<QString, IResolver *> m_data = {{"telemetry", &m_impl_telemetry_resolver}};
};
