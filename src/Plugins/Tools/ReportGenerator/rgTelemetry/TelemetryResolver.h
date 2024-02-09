#pragma once

#include "rgInterfaces/IResolver.h"
#include <optional>
#include <QtCore>

class TelemetryFuncRegistry;

/**
 * @brief The task of the class is to return a string value that will be inserted into the final report
 * 
 */
class TelemetryResolver : public IResolver
{
public:
    TelemetryResolver();

    virtual std::optional<QString> get_value(QString command) override;

private:
    TelemetryFuncRegistry &m_functions;
};
