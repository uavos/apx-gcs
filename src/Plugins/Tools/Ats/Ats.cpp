#include "Ats.h"

static constexpr const char *ats_unit_active = "ACTIVE";

Ats::Ats(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("ATS"),
           tr("Antenna tracking system"),
           Group,
           "antenna")
{
    f_ats_enabled = new Fact(this, "enable", tr("Enable"), tr("Enable ATS"), Fact::Bool, "link");

    f_ats_unit = new Fact(this, "unit", tr("Unit"), tr("Unit to track"), Text, "drone");
    f_ats_unit->setOpt("editor", "EditorOption.qml"); // non-editable list
    f_ats_unit->setValue(ats_unit_active);

    auto fleet = Fleet::instance();
    connect(fleet, &Fleet::unitRegistered, this, &Ats::updateUnitsList);
    connect(fleet, &Fact::itemRemoved, this, &Ats::updateUnitsList);
    updateUnitsList();

    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    _ats_timer.start();
}

void Ats::updateUnitsList()
{
    QStringList list{ats_unit_active};
    for (auto i : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(i);
        if (!unit || !unit->isIdentified() || unit->isGroundControl())
            continue;
        if (!list.contains(unit->title()))
            list.append(unit->title());
    }
    f_ats_unit->setEnumStrings(list);

    // selected unit is removed from the fleet
    if (!list.contains(f_ats_unit->value().toString()))
        f_ats_unit->setValue(ats_unit_active);
}

Unit *Ats::trackedUnit() const
{
    auto fleet = Fleet::instance();

    const auto callsign = f_ats_unit->value().toString();
    if (callsign == ats_unit_active)
        return fleet->current();

    for (auto i : fleet->facts()) {
        auto unit = qobject_cast<Unit *>(i);
        if (!unit || !unit->isIdentified() || unit->isGroundControl())
            continue;
        if (unit->title() == callsign)
            return unit;
    }
    return nullptr;
}

void Ats::onAtsTimer()
{
    if (!f_ats_enabled->value().toBool()) {
        return;
    }

    auto unit = trackedUnit();
    if (!unit) {
        return;
    }

    PData *pdata{};
    if (unit->isGroundControl()) {
        auto protocol = unit->protocol();
        if (protocol) {
            pdata = protocol->data();
            if (pdata) {
                pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_manual);
            }
        }
        return;
    }

    auto gcs = Fleet::instance()->gcs();
    if (gcs && gcs->protocol()) {
        pdata = gcs->protocol()->data();
        if (pdata) {
            QGeoCoordinate uav = unit->coordinate();
            QVariantList value;
            value << uav.latitude();
            value << uav.longitude();
            value << uav.altitude();
            pdata->sendValue(mandala::cmd::nav::ats::uid, value);
            pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_track);
        }
    }
}
