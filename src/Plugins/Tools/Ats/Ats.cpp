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
    connect(f_ats_enabled, &Fact::valueChanged, this, &Ats::onEnabledChanged);

    f_ats_unit = new Fact(this, "unit", tr("Unit"), tr("Unit to track"), Text, "drone");
    f_ats_unit->setOpt("editor", "EditorOption.qml"); // non-editable list
    f_ats_unit->setValue(ats_unit_active);

    f_units = new Fact(this,
                       "units",
                       tr("ATS units"),
                       tr("Mode and RSSI window of each ATS"),
                       Fact::Group,
                       "antenna");

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
    connect(fleet, &Fleet::currentChanged, this, &Ats::updateTrackedUnit);
    connect(f_ats_unit, &Fact::valueChanged, this, &Ats::updateTrackedUnit);
    updateUnitsList();

    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    _ats_timer.start();

    loadQml("qrc:/" PLUGIN_NAME "/AtsPlugin.qml");
}

void Ats::onEnabledChanged()
{
    // the timer sends the modes while enabled, switch all ATS off once when disabled
    if (f_ats_enabled->value().toBool())
        return;
    for (auto unit : gcsUnits())
        sendMode(unit, mandala::ats_mode_off);
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

    updateAtsUnits();
    updateTrackedUnit();
}

// mode and RSSI window per GCS unit
void Ats::updateAtsUnits()
{
    auto fleet = Fleet::instance();

    for (auto i : fleet->facts()) {
        auto unit = qobject_cast<Unit *>(i);
        if (!unit || !unit->isIdentified() || !unit->isGroundControl())
            continue;
        if (_units.contains(unit))
            continue;

        AtsUnit u;
        u.group = new Fact(f_units, unit->name(), unit->title(), "", Fact::Group, "antenna");
        u.group->bindProperty(unit, "title", true);

        u.mode = new Fact(u.group,
                          "mode",
                          tr("Mode"),
                          tr("AUTO - by the tracked unit, TRACK or MANUAL - fixed"),
                          Fact::Enum,
                          "swap-horizontal");
        u.mode->setEnumStrings({"AUTO", "TRACK", "MANUAL"});
        u.mode->setValue(AUTO);

        // mode text in the group row
        auto group = u.group;
        auto mode = u.mode;
        connect(mode, &Fact::valueChanged, group, [group, mode]() {
            group->setValue(mode->text());
        });
        group->setValue(mode->text());

        u.rssi = new Fact(u.group,
                          "rssi",
                          tr("RSSI"),
                          tr("Show RSSI window"),
                          Fact::Bool,
                          "signal-variant");

        QVariantMap opts;
        opts.insert("ats", QVariant::fromValue(this));
        opts.insert("fact", QVariant::fromValue(u.rssi));
        opts.insert("modeFact", QVariant::fromValue(u.mode));
        opts.insert("unit", QVariant::fromValue(unit));
        u.window = App::loadQml("qrc:/" PLUGIN_NAME "/rssi/RssiWindow.qml", opts);

        _units.insert(unit, u);
    }

    // units removed from the fleet
    for (auto unit : _units.keys()) {
        if (fleet->facts().contains(unit))
            continue;
        auto u = _units.take(unit);
        if (u.window)
            u.window->deleteLater();
        u.group->deleteFact();
    }
}

void Ats::updateTrackedUnit()
{
    auto unit = findTrackedUnit();
    if (_tracked == unit)
        return;
    _tracked = unit;
    emit trackedUnitChanged();
}

// GCS unit selected by the operator: this ATS is put into manual in AUTO mode
Unit *Ats::selectedGcs() const
{
    auto current = Fleet::instance()->current();
    return (current && current->isGroundControl()) ? current : nullptr;
}

Unit *Ats::findTrackedUnit() const
{
    auto fleet = Fleet::instance();

    const auto callsign = f_ats_unit->value().toString();
    if (callsign == ats_unit_active) {
        // a selected GCS unit does not change the tracked unit
        if (selectedGcs())
            return _tracked;
        return fleet->current();
    }

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

    auto gcs = selectedGcs();
    auto unit = trackedUnit();
    bool tracking = unit && !unit->isGroundControl();

    if (tracking) {
        QGeoCoordinate uav = unit->coordinate();
        QVariantList value;
        value << uav.latitude();
        value << uav.longitude();
        value << uav.altitude();
        sendValues(value);
    }

    for (auto ats : gcsUnits()) {
        int mode = _units.contains(ats) ? _units.value(ats).mode->value().toInt() : AUTO;
        switch (mode) {
        case MANUAL:
            sendMode(ats, mandala::ats_mode_manual);
            break;
        case TRACK:
            sendMode(ats, mandala::ats_mode_track);
            break;
        default: // AUTO: selected GCS unit is manual, the others track the unit
            if (ats == gcs)
                sendMode(ats, mandala::ats_mode_manual);
            else if (tracking)
                sendMode(ats, mandala::ats_mode_track);
            break;
        }
    }
}

QList<Unit *> Ats::gcsUnits() const
{
    QList<Unit *> list;
    for (auto i : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(i);
        if (unit && unit->isGroundControl() && unit->protocol())
            list.append(unit);
    }
    return list;
}

// position to all GCS units in the fleet
void Ats::sendValues(const QVariantList &value)
{
    for (auto unit : gcsUnits()) {
        auto pdata = unit->protocol()->data();
        if (pdata)
            pdata->sendValue(mandala::cmd::nav::ats::uid, value);
    }
}

void Ats::sendMode(Unit *unit, uint8_t mode)
{
    if (!unit || !unit->protocol())
        return;
    auto pdata = unit->protocol()->data();
    if (pdata)
        pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mode);
}
