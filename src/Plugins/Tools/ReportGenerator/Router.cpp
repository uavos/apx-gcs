#include "Router.h"

namespace RG {

std::optional<QString> Router::resolvePath(const QString &command) const
{
    const auto routes = command.split("/");

    if (routes.size() < 2)
        return std::nullopt;

    const auto &resolver = routes[0];
    const auto &function_name = routes[1];

    if (auto it = m_resolvers_map.find(resolver); it != m_resolvers_map.end()) {
        return it.value()->getValue(function_name);
    }

    return std::nullopt;
}

}; // namespace ReportGenerator
