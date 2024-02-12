#include "Router.h"

std::optional<QString> Router::resolve_path(const QString &command) const
{
    const auto routes = command.split("/");

    if (routes.size() < 2)
        return std::nullopt;

    const auto &resolver = routes[0];
    const auto &function_name = routes[1];

    if (auto it = m_resolvers_map.find(resolver); it != m_resolvers_map.end()) {
        return it.value()->get_value(function_name);
    }

    return std::nullopt;
}
