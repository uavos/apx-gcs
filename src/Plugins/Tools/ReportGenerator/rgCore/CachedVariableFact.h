#pragma once

#include "rgCore/FunctionResult.h"
#include "rgUtils/CachedFunction.h"
#include <QtCore>

namespace RG {

class CachedVariableFact : public CachedFunction<FunctionResult>
{
public:
    CachedVariableFact(std::function<FunctionResult()> func);
};

} // namespace RG
