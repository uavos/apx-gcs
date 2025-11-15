#include "Swarm.h"

Swarm::Swarm(Fact *parent)
    : Fact(parent, QString(PLUGIN_NAME).toLower(), tr("Swarm"), tr("Swarm management"), Group, "arch")
{
    f_swarm_enabled = new Fact(this, "enable", tr("Enable"), tr("Enable Swarm"), Fact::Bool, "link");
    f_swarm_master = new Fact(this,
                              "callsign",
                              tr("Unit Callsign"),
                              tr("Unit callsign of the swarm master"),
                              Text | PersistentValue);
    f_swarm_master->setDefaultValue("M-97");

    _swarm_timer.setInterval(500);
    connect(&_swarm_timer, &QTimer::timeout, this, &Swarm::onSwarmTimer);
    _swarm_timer.start();
}

void Swarm::onSwarmTimer()
{
    if (!f_swarm_enabled->value().toBool()) {
        return;
    }

    for (auto i : Fleet::instance()->facts()) {
        Unit *m_unit = static_cast<Unit *>(i);

        if (m_unit && m_unit->isIdentified()) {
            if (m_unit->title() == f_swarm_master->value().toString()) {
                //Tx data;
                //ID
                //SwarmPub::_id.src = conf::data.shiva.swarm.id;
                //SwarmPub::_id.type = mandala::bundle::swarm_data;
                //SwarmPub::_id.ext = 0;

                //Data
                //SwarmPub::_data.cmd_airspeed = m.cmd_airspeed;
                //SwarmPub::_data.cmd_altitude = m.cmd_altitude;
                //SwarmPub::_data.cmd_bearing = m.cmd_bearing;
                //SwarmPub::_data.cmd_lacc = RegHdg::lateral_acc();

                //SwarmPub::_data.bearing = m.bearing;

                //SwarmPub::_data.cmd_llh[0] = m_lat;
                //SwarmPub::_data.cmd_llh[1] = m_lon;
                //SwarmPub::_data.cmd_llh[2] = m_hmsl;

                //swarm id
                _swarm_data.id.src = 0;
                _swarm_data.id.type = mandala::bundle::swarm_data;
                _swarm_data.id.ext = 0;

                Mandala *mandala = m_unit->f_mandala;

                if (!mandala) {
                    break;
                }

                //swarm data
                _swarm_data.cmd_airspeed
                    = mandala->fact(mandala::cmd::nav::pos::airspeed::uid)->value().toFloat();

                _swarm_data.cmd_altitude
                    = mandala->fact(mandala::cmd::nav::pos::altitude::uid)->value().toFloat();

                _swarm_data.cmd_bearing = qDegreesToRadians(
                    mandala->fact(mandala::cmd::nav::pos::bearing::uid)->value().toFloat());

                _swarm_data.cmd_lacc
                    = mandala->fact(mandala::est::env::usrf::f15::uid)->value().toFloat();

                _swarm_data.bearing = qDegreesToRadians(
                    mandala->fact(mandala::est::nav::pos::bearing::uid)->value().toFloat());

                _swarm_data.cmd_llh[0] = mandala::deg_to_a32(
                    mandala->fact(mandala::est::nav::pos::lat::uid)->value().toDouble());

                _swarm_data.cmd_llh[1] = mandala::deg_to_a32(
                    mandala->fact(mandala::est::nav::pos::lon::uid)->value().toDouble());

                _swarm_data.cmd_llh[2]
                    = mandala->fact(mandala::est::nav::pos::hmsl::uid)->value().toFloat();

                swarm_update = true;
                break;
            }
        }
    }

    for (auto i : Fleet::instance()->facts()) {
        Unit *s_unit = static_cast<Unit *>(i);

        if (s_unit && s_unit->isIdentified()) {
            if (s_unit->title() == f_swarm_master->value().toString()) {
                continue;
            }

            if (swarm_update) {
                PData *pdata = s_unit->protocol()->data();
                if (pdata) {
                    QByteArray ba;
                    ba.resize(sizeof(swarm::swarm_s));
                    memcpy(ba.data(), &_swarm_data, sizeof(_swarm_data));

                    QVariant value = ba;
                    pdata->sendValue(mandala::cmd::nav::swarm::uid, value);
                }
            }
        }
    }

    swarm_update = false;
}
