#pragma once

#include <QtCore>

namespace RG {

enum class ReturnStatus { Success, Error, WrongParameter, CantAccessTelemetryParameter };

class FunctionResult
{
public:
    FunctionResult() = default;
    FunctionResult(QVariant value, ReturnStatus status = ReturnStatus::Success);
    FunctionResult(ReturnStatus status);

    ReturnStatus &status();
    QVariant &value();

private:
    ReturnStatus m_status;
    QVariant m_value;
};

} // namespace RG
