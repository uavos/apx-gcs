#include "CachedVariableFact.h"

RG::CachedVariableFact::CachedVariableFact(std::function<FunctionResult()> func)
    : CachedFunction{func}
{}
