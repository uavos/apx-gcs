#pragma once

#include <optional>
#include <QtCore>

class TelemetryFuncRegistry;

class TelemetryResolver
{
public:
    TelemetryResolver();

    std::optional<QString> get_value(QString field_name);

private:
    TelemetryFuncRegistry &m_functions;
};
