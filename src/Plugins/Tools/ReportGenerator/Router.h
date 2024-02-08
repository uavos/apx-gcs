#pragma once

#include "rgTelemetry/TelemetryResolver.h"
#include <optional>
#include <QtCore>

class Router
{
public:
    Router();

    std::optional<QString> resolve_path(QString command);

private:
    TelemetryResolver m_telemetry_resolver;
};
