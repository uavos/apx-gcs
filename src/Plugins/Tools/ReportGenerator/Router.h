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
    Router() = default;

    std::optional<QString> resolve_path(const QString &command) const;

private:
    TelemetryResolver m_impl_telemetry_resolver;
    QHash<QString, IResolver *> m_resolvers_map = {{"telemetry", &m_impl_telemetry_resolver}};
};
