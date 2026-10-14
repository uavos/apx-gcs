#pragma once

#include <QPointer>
#include <QTimer>

#include "App/AppGcs.h"
#include "Fact/Fact.h"
#include "Fleet/Unit.h"

class Ats : public Fact
{
    Q_OBJECT

    // unit tracked by ATS: the selected one or the active unit
    Q_PROPERTY(Unit *trackedUnit READ trackedUnit NOTIFY trackedUnitChanged)

public:
    explicit Ats(Fact *parent = nullptr);

    Unit *trackedUnit() const { return _tracked; }

    // mode of each ATS set by the operator
    enum AtsMode { AUTO = 0, TRACK, MANUAL };

private:
    Fact *f_ats_enabled;
    Fact *f_ats_unit;
    Fact *f_units; // group with mode and RSSI window toggle per GCS unit

    Fact *f_overlay;
    Fact *f_show_beam;
    Fact *f_beam_distance;

    QTimer _ats_timer;

    QPointer<Unit> _tracked;
    Unit *findTrackedUnit() const;
    Unit *selectedGcs() const;

    struct AtsUnit
    {
        Fact *group{};
        Fact *mode{};
        Fact *rssi{};
        QObject *window{};
    };
    QHash<Unit *, AtsUnit> _units;
    void updateAtsUnits();

    QList<Unit *> gcsUnits() const;
    void sendValues(const QVariantList &value);
    void sendMode(Unit *unit, uint8_t mode);

private slots:
    void onEnabledChanged();
    void updateUnitsList();
    void updateTrackedUnit();
    void onAtsTimer();

signals:
    void trackedUnitChanged();
};
