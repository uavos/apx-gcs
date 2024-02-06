#pragma once

#include "./Fact/Fact.h"

class ReportGenerator : public Fact
{
    Q_OBJECT

    Fact *f_launch;

public:
    explicit ReportGenerator(Fact *parent = nullptr);

private slots:
    void test();
};
