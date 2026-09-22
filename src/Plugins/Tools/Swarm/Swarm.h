#pragma once

#include <QTimer>
#include <QTimerEvent>
#include <QtNetwork>

#include "App/AppGcs.h"
#include "Fact/Fact.h"
#include "Fleet/Unit.h"

#include <serial/CobsDecoder.h>
#include <serial/CobsEncoder.h>

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

    // UDP relay: local apx.sim bus COBS stream -> swarm bundles (pid + payload)
    // -> COBS -> uplink port of apx.sim on other computers, the same as from GCU
    Fact *f_udp_enabled;
    Fact *f_udp_local;
    Fact *f_udp_sims;

    QUdpSocket _udp_local;
    QList<QPair<QHostAddress, quint16>> _sims;

    CobsDecoder<> _dec_local;
    CobsEncoder<> _enc;

    // recently sent packets, to break loops through remote sims
    QElapsedTimer _time;
    QList<QPair<qint64, QByteArray>> _sent;

    void udpBind();
    void udpUpdateSims();

    void sendToSims(const QByteArray &packet);
    void dropExpired();
    bool wasSent(const QByteArray &packet);

private slots:
    void onSwarmTimer();
    void udpReadLocal();
};
