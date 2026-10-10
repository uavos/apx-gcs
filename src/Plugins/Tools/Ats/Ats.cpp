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

    f_overlay = new Fact(this,
                         "overlay",
                         tr("Overlay"),
                         tr("Map overlay settings"),
                         Fact::Group,
                         "layers");

    f_show_beam = new Fact(f_overlay,
                           "show_beam",
                           tr("Show beam"),
                           tr("Show beam line on map"),
                           Fact::Bool | Fact::PersistentValue,
                           "ray-start");
    f_show_beam->setDefaultValue(false);

    f_beam_distance = new Fact(f_overlay,
                               "beam_distance",
                               tr("Beam distance"),
                               tr("Beam cone length in km"),
                               Fact::Int | Fact::PersistentValue,
                               "arrow-expand-horizontal");
    f_beam_distance->setMin(5);
    f_beam_distance->setMax(300);
    f_beam_distance->setUnits("km");
    f_beam_distance->setDefaultValue(30);

    auto fleet = Fleet::instance();
    connect(fleet, &Fleet::unitRegistered, this, &Ats::updateUnitsList);
    connect(fleet, &Fact::itemRemoved, this, &Ats::updateUnitsList);
    updateUnitsList();

    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    _ats_timer.start();

    loadQml("qrc:/" PLUGIN_NAME "/AtsPlugin.qml");
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

    if (unit->isGroundControl()) {
        sendMode(mandala::ats_mode_manual);
        return;
    }

    QGeoCoordinate uav = unit->coordinate();
    QVariantList value;
    value << uav.latitude();
    value << uav.longitude();
    value << uav.altitude();
    sendValues(value);
    sendMode(mandala::ats_mode_track);
}

// send to all GCS units in the fleet
void Ats::sendValues(const QVariantList &value)
{
    for (auto i : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(i);
        if (!unit || !unit->isGroundControl() || !unit->protocol())
            continue;
        auto pdata = unit->protocol()->data();
        if (pdata)
            pdata->sendValue(mandala::cmd::nav::ats::uid, value);
    }
}

void Ats::sendMode(uint8_t mode)
{
    for (auto i : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(i);
        if (!unit || !unit->isGroundControl() || !unit->protocol())
            continue;
        auto pdata = unit->protocol()->data();
        if (pdata)
            pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mode);
    }
}
