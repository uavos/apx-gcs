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

    f_rssi = new Fact(this,
                      "rssi",
                      tr("RSSI"),
                      tr("RSSI heatmap by azimuth and elevation"),
                      Fact::Bool,
                      "signal-variant");

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
    f_beam_distance->setMax(100);
    f_beam_distance->setUnits("km");
    f_beam_distance->setDefaultValue(30);

    f_show_compass = new Fact(f_overlay,
                              "show_compass",
                              tr("Show compass"),
                              tr("Show compass circle with degrees on map"),
                              Fact::Bool | Fact::PersistentValue,
                              "compass-outline");
    f_show_compass->setDefaultValue(false);

    f_compass_radius = new Fact(f_overlay,
                                "compass_radius",
                                tr("Compass radius"),
                                tr("Compass circle radius in km"),
                                Fact::Int | Fact::PersistentValue,
                                "circle-outline");
    f_compass_radius->setMin(1);
    f_compass_radius->setMax(100);
    f_compass_radius->setUnits("km");
    f_compass_radius->setDefaultValue(5);

    f_bias = new Fact(this,
                      "bias",
                      tr("Bias"),
                      tr("Antenna bias adjustment"),
                      Fact::Group,
                      "tune-vertical");

    f_bias_yaw = new Fact(f_bias,
                          "yaw",
                          tr("Azimuth"),
                          tr("Azimuth bias offset"),
                          Fact::Float,
                          "compass-outline");
    f_bias_yaw->setMin(-180);
    f_bias_yaw->setMax(180);
    f_bias_yaw->setUnits("deg");
    f_bias_yaw->setDefaultValue(0);

    f_bias_pitch = new Fact(f_bias,
                            "pitch",
                            tr("Elevation"),
                            tr("Elevation bias offset"),
                            Fact::Float,
                            "angle-acute");
    f_bias_pitch->setMin(-90);
    f_bias_pitch->setMax(90);
    f_bias_pitch->setUnits("deg");
    f_bias_pitch->setDefaultValue(0);

    // Bias is adjusted only via RSSI plugin window
    f_bias->setVisible(false);

    // Debounce bias updates to avoid flooding the link on rapid changes
    _bias_timer.setInterval(200);
    _bias_timer.setSingleShot(true);
    connect(&_bias_timer, &QTimer::timeout, this, &Ats::sendBias);
    connect(f_bias_yaw, &Fact::valueChanged, &_bias_timer, qOverload<>(&QTimer::start));
    connect(f_bias_pitch, &Fact::valueChanged, &_bias_timer, qOverload<>(&QTimer::start));

    // Run position updates only while ATS is enabled
    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    connect(f_ats_enabled, &Fact::valueChanged, this, &Ats::onEnabledChanged);

    // Mode is switched once per selection change: GCS - manual, UAV - track
    connect(Fleet::instance(), &Fleet::unitSelected, this, [this]() { applyMode(); });

    // Sync enable state with actual ATS mode reported by GCS unit
    connect(Fleet::instance(), &Fleet::gcsChanged, this, &Ats::onGcsChanged);
    onGcsChanged();

    f_rssi->loadQml("qrc:/" PLUGIN_NAME "/AtsRssi.qml");

    loadQml("qrc:/" PLUGIN_NAME "/AtsPlugin.qml");
}

void Ats::onEnabledChanged()
{
    bool on = f_ats_enabled->value().toBool();

    on ? _ats_timer.start() : _ats_timer.stop();

    // do not send mode when just reflecting the actual state
    if (_syncing)
        return;

    on ? applyMode() : sendMode(mandala::ats_mode_off);
}

void Ats::onGcsChanged()
{
    if (_mode_conn)
        disconnect(_mode_conn);

    auto gcs = Fleet::instance()->gcs();
    if (!gcs || !gcs->f_mandala)
        return;

    auto f_mode = gcs->f_mandala->fact(mandala::cmd::nav::ats::mode::uid);
    if (!f_mode)
        return;

    _mode_conn = connect(f_mode, &Fact::valueChanged, this, &Ats::syncEnabled);
    syncEnabled();
}

void Ats::syncEnabled()
{
    auto gcs = Fleet::instance()->gcs();
    if (!gcs || !gcs->f_mandala)
        return;

    auto f_mode = gcs->f_mandala->fact(mandala::cmd::nav::ats::mode::uid);
    if (!f_mode)
        return;

    bool active = f_mode->value().toUInt() != mandala::ats_mode_off;
    if (f_ats_enabled->value().toBool() == active)
        return;

    _syncing = true;
    f_ats_enabled->setValue(active);
    _syncing = false;
}

void Ats::onAtsTimer()
{
    // Stream UAV position only; mode is switched once by applyMode()
    auto current = Fleet::instance()->current();
    if (!current || current->isGroundControl())
        return;

    QGeoCoordinate uav = current->coordinate();
    QVariantList value;
    value << mandala::bundle::ats_pos;
    value << uav.latitude();
    value << uav.longitude();
    value << uav.altitude();

    sendValues(value);
}

void Ats::applyMode()
{
    if (!f_ats_enabled->value().toBool())
        return;

    auto current = Fleet::instance()->current();
    if (!current)
        return;

    current->isGroundControl() ? sendMode(mandala::ats_mode_manual)
                               : sendMode(mandala::ats_mode_track);
}

void Ats::setModeManual()
{
    sendMode(mandala::ats_mode_manual);
}

void Ats::setModeTrack()
{
    sendMode(mandala::ats_mode_track);
}

void Ats::setModeSearch()
{
    sendMode(mandala::ats_mode_search);
}

void Ats::sendSearch(float yaw, float pitch, int time)
{
    // Scan sector (az/el range around current direction) and dwell time.
    // The scan itself is performed by the modem.
    QVariantList search;
    search << mandala::bundle::ats_scan;
    search << qDegreesToRadians(yaw);
    search << qDegreesToRadians(pitch);
    search << qBound(0, time, 255);

    sendValues(search);
}

void Ats::sendBias()
{
    QVariantList bias;
    bias << mandala::bundle::ats_bias;
    bias << qDegreesToRadians(f_bias_yaw->value().toFloat());
    bias << qDegreesToRadians(f_bias_pitch->value().toFloat());

    sendValues(bias);
}

void Ats::sendValues(const QVariantList &value)
{
    for (auto fact : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(fact);
        if (!unit || !unit->isGroundControl())
            continue;
        if (!unit->protocol())
            continue;
        PData *pdata = unit->protocol()->data();
        if (!pdata)
            continue;

        pdata->sendValue(mandala::cmd::nav::ats::uid, value);
    }
}

void Ats::sendMode(uint8_t mode)
{
    for (auto fact : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(fact);
        if (!unit || !unit->isGroundControl())
            continue;
        if (!unit->f_mandala)
            continue;

        auto f_mode = unit->f_mandala->fact(mandala::cmd::nav::ats::mode::uid);
        if (f_mode)
            f_mode->setValue(mode);
    }
}
