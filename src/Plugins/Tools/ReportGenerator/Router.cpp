#include "Router.h"

Router::Router() {}

std::optional<QString> Router::resolve_path(QString command)
{
    auto routes = command.split("/");

    if (routes.size() < 2)
        return std::nullopt;

    auto &resolver = routes[0];
    auto &function_name = routes[1];

    auto it = m_resolvers_map.find(resolver);

    if (it == m_resolvers_map.end())
        return std::nullopt;

    return it.value()->get_value(function_name);
}
