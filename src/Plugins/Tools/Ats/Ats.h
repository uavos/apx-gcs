#pragma once

#include <QTimer>

#include "App/AppGcs.h"
#include "Fact/Fact.h"
#include "Fleet/Unit.h"

class Ats : public Fact
{
    Q_OBJECT

public:
    explicit Ats(Fact *parent = nullptr);

    Q_INVOKABLE void setModeManual();
    Q_INVOKABLE void setModeTrack();
    Q_INVOKABLE void setModeSearch();
    Q_INVOKABLE void sendSearch(float yaw, float pitch, int time);

private:
    void sendValues(const QVariantList &value);
    void sendMode(uint8_t mode);
    void applyMode();
    void sendBias();

    Fact *f_ats_enabled;
    Fact *f_overlay;
    Fact *f_show_beam;
    Fact *f_beam_distance;
    Fact *f_show_compass;
    Fact *f_compass_radius;

    Fact *f_bias;
    Fact *f_bias_yaw;
    Fact *f_bias_pitch;
    Fact *f_rssi;

    QTimer _ats_timer;
    QTimer _bias_timer;

    bool _syncing{false};
    QMetaObject::Connection _mode_conn;

private slots:
    void onAtsTimer();
    void onEnabledChanged();
    void onGcsChanged();
    void syncEnabled();
};
