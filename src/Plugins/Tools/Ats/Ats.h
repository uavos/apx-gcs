#pragma once

#include <QTimer>
#include <QTimerEvent>

#include "App/AppGcs.h"
#include "Fact/Fact.h"
#include "Vehicles/Vehicle.h"

class Ats : public Fact
{
    Q_OBJECT

public:
    explicit Ats(Fact *parent = nullptr);

private:
    Fact *f_ats_enabled;

    QTimer _ats_timer;

private slots:
    void onAtsTimer();
};
