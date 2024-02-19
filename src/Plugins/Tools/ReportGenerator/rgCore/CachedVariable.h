#pragma once

#include "rgCore/FunctionResult.h"
#include "rgUtils/CachedFunction.h"
#include <QtCore>

namespace RG {

class CachedVariable : public CachedFunction<FunctionResult>
{
public:
    CachedVariable(std::function<FunctionResult()> func);
};

} // namespace RG
