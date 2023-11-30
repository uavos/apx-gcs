#pragma once

#include "Fact/Fact.h"

class VcpForwarding : public Fact
{
    Q_OBJECT

public:
    explicit VcpForwarding(Fact *parent = nullptr);
};
