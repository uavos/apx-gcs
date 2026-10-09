#pragma once

#include <QTimer>
#include <QTimerEvent>

#include "App/AppGcs.h"
#include "Fact/Fact.h"
#include "Fleet/Unit.h"

class Ats : public Fact
{
    Q_OBJECT

public:
    explicit Ats(Fact *parent = nullptr);

private:
    Fact *f_ats_enabled;
    Fact *f_ats_unit;

    QTimer _ats_timer;

    Unit *trackedUnit() const;

private slots:
    void updateUnitsList();
    void onAtsTimer();
};
