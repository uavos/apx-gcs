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

    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    _ats_timer.start();
}

void Ats::onAtsTimer()
{
    if (!f_ats_enabled->value().toBool()) {
        return;
    }

    auto current = Fleet::instance()->current();
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

    if (Fleet::instance()->gcs()->protocol()) {
        pdata = Fleet::instance()->gcs()->protocol()->data();
        if (pdata) {
            QGeoCoordinate uav = current->coordinate();
            QVariantList value;
            value << uav.latitude();
            value << uav.longitude();
            value << uav.altitude();
            pdata->sendValue(mandala::cmd::nav::ats::uid, value);
            pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_track);
        }
    }
}
