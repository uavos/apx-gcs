#include "FunctionResult.h"

RG::FunctionResult::FunctionResult(QVariant value, ReturnStatus status)
    : m_value{value}
    , m_status{status}
{}

RG::FunctionResult::FunctionResult(ReturnStatus status)
    : m_value{}
    , m_status{status}
{}

RG::ReturnStatus &RG::FunctionResult::status()
{
    return m_status;
}

QVariant &RG::FunctionResult::value()
{
    return m_value;
}
