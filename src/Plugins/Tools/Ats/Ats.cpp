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
                          Fact::Float | Fact::PersistentValue,
                          "compass-outline");
    f_bias_yaw->setMin(-180);
    f_bias_yaw->setMax(180);
    f_bias_yaw->setUnits("deg");
    f_bias_yaw->setDefaultValue(0);

    f_bias_pitch = new Fact(f_bias,
                            "pitch",
                            tr("Elevation"),
                            tr("Elevation bias offset"),
                            Fact::Float | Fact::PersistentValue,
                            "angle-acute");
    f_bias_pitch->setMin(-90);
    f_bias_pitch->setMax(90);
    f_bias_pitch->setUnits("deg");
    f_bias_pitch->setDefaultValue(0);

    connect(f_bias_yaw, &Fact::valueChanged, this, &Ats::onBiasChanged);
    connect(f_bias_pitch, &Fact::valueChanged, this, &Ats::onBiasChanged);

    _ats_timer.setInterval(100);
    connect(&_ats_timer, &QTimer::timeout, this, &Ats::onAtsTimer);
    _ats_timer.start();

    loadQml("qrc:/" PLUGIN_NAME "/AtsPlugin.qml");
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

    // If current unit is GCS, send manual mode to it only
    if (current->isGroundControl()) {
        auto protocol = current->protocol();
        if (protocol) {
            PData *pdata = protocol->data();
            if (pdata) {
                pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_manual);
            }
        }
        return;
    }

    // Send track command to all GCS units
    QGeoCoordinate uav = current->coordinate();
    QVariantList value;
    value << mandala::bundle::ats_pos;
    value << uav.latitude();
    value << uav.longitude();
    value << uav.altitude();

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
        pdata->sendValue(mandala::cmd::nav::ats::mode::uid, mandala::ats_mode_track);
    }
}

void Ats::onBiasChanged()
{
    QVariantList bias;
    bias << mandala::bundle::ats_bias;
    bias << 0.0f; // roll
    bias << qDegreesToRadians(f_bias_pitch->value().toFloat());
    bias << qDegreesToRadians(f_bias_yaw->value().toFloat());

    for (auto fact : Fleet::instance()->facts()) {
        auto unit = qobject_cast<Unit *>(fact);
        if (!unit || !unit->isGroundControl())
            continue;
        if (!unit->protocol())
            continue;
        PData *pdata = unit->protocol()->data();
        if (!pdata)
            continue;
        pdata->sendValue(mandala::cmd::nav::ats::uid, bias);
    }
}
