#include "Ats.h"

Ats::Ats(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("ATS"),
           tr("Antenna tracking system"),
           Group,
           "antenna")
{
    f_ats_enabled = new Fact(this, "enable", tr("Enable"), tr("Enable ATS"), Fact::Bool, "link");
    f_ats_vcpid = new Fact(this,
                           "vcpid",
                           tr("VCP"),
                           tr("Virtual Port Number (ID)"),
                           Fact::Int | Fact::PersistentValue,
                           "numeric");
    f_ats_vcpid->setMin(0);
    f_ats_vcpid->setMax(255);
    f_ats_vcpid->setValue(QSettings().value("ats_vcpid"));
    connect(f_ats_vcpid, &Fact::textChanged, this, [&]() {
        QSettings().setValue("ats_vcpid", f_ats_vcpid->text());
    });

    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    _ats_timer.start();
}

void Ats::onAtsTimer()
{
    if (!f_ats_enabled->value().toBool()) {
        return;
    }

    Vehicle *current = Vehicles::instance()->current();
    if (!current) {
        return;
    }

    PData *pdata{};
    if (current->isGroundControl()) {
        auto protocol = current->protocol();
        if (protocol) {
            pdata = current->protocol()->data();
            if (pdata) {
                pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_manual);
            }
        }
        return;
    }

    QGeoCoordinate uav_pos = current->coordinate();
    if (Vehicles::instance()->gcs()->protocol()) {
        float lat = uav_pos.latitude();
        float lon = uav_pos.longitude();
        float hmsl = uav_pos.altitude();

        QByteArray ba;
        ba.append(reinterpret_cast<const char *>(&lat), sizeof(float));
        ba.append(reinterpret_cast<const char *>(&lon), sizeof(float));
        ba.append(reinterpret_cast<const char *>(&hmsl), sizeof(float));

        pdata = Vehicles::instance()->gcs()->protocol()->data();
        if (pdata) {
            pdata->sendSerial(f_ats_vcpid->value().toInt(), ba);
            pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_track);
        }

        //test
        //_pdata->sendValue(mandala::est::env::usrf::f1::uid, lat);
        //_pdata->sendValue(mandala::est::env::usrf::f2::uid, lon);
        //_pdata->sendValue(mandala::est::env::usrf::f3::uid, hmsl);
    }
}
