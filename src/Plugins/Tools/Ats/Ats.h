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
    Fact *f_overlay;
    Fact *f_show_beam;
    Fact *f_beam_distance;
    Fact *f_show_compass;
    Fact *f_compass_radius;

    Fact *f_bias;
    Fact *f_bias_yaw;
    Fact *f_bias_pitch;

    QTimer _ats_timer;

private slots:
    void onAtsTimer();
    void onBiasChanged();
};
