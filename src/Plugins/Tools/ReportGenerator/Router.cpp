#include "Router.h"

Router::Router() {}

std::optional<QString> Router::resolve_path(QString command)
{
    auto routes = command.split("/");
    
    if(routes.size()<2) return std::nullopt;

    if(routes[0] == "telemetry") return m_telemetry_resolver.get_value(routes[1]);

    return std::nullopt;
}
