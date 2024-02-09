#pragma once

#include <optional>
#include <QtCore>

class TelemetryFuncRegistry;

/**
 * @brief The task of the class is to return a string value that will be inserted into the final report
 * 
 */
class TelemetryResolver
{
public:
    TelemetryResolver();

    std::optional<QString> get_value(QString field_name);

private:
    TelemetryFuncRegistry &m_functions;
};
