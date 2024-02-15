#include "CachedVariable.h"

RG::CachedVariable::CachedVariable(std::function<FunctionResult()> func)
    : CachedFunction{func}
{}
