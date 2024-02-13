#pragma once

#include "rgInterfaces/IResolver.h"
#include <optional>
#include <QtCore>

namespace ReportGenerator {

class TelemetryFunctions;

/**
 * @brief The task of the class is to return a string value that will be inserted into the final report
 * 
 */
class TelemetryResolver : public IResolver
{
public:
    TelemetryResolver();

    virtual std::optional<QString> getValue(QString command) override;

private:
    TelemetryFunctions &m_functions;
};

};
