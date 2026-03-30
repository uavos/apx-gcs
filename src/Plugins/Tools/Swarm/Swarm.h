#pragma once

#include <QTimer>
#include <QTimerEvent>

#include "App/AppGcs.h"
#include "Fact/Fact.h"
#include "Fleet/Unit.h"

namespace swarm {

enum {
    swarm_followme = 1,
    swarm_data = 2,
};

#pragma pack(1)
union swarmid_s {
    uint16_t raw;
    struct
    {
        uint16_t src : 11;
        uint8_t type : 4;
        uint8_t ext : 1;
    };
};

struct swarm_s
{
    swarmid_s id;
    float cmd_airspeed; // m/s
    float cmd_altitude; // m
    float cmd_bearing;  // rad
    float cmd_lacc;     // m/s^2
    float bearing;      // rad
    int32_t cmd_llh[3]; // 1e-7 degrees, 1e-3 meters
};
#pragma pack()

} // namespace swarm

class Swarm : public Fact
{
    Q_OBJECT

public:
    explicit Swarm(Fact *parent = nullptr);

private:
    Fact *f_swarm_enabled;
    Fact *f_swarm_master;

    QTimer _swarm_timer;

    swarm::swarm_s _swarm_data;

    bool swarm_update{false};

private slots:
    void onSwarmTimer();
};
