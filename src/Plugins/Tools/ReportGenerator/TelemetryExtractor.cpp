#include "TelemetryExtractor.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryReader.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

TelemetryExtractor::TelemetryExtractor() {}

void TelemetryExtractor::sync()
{
    auto telemetry = Vehicles::instance()->f_replay->f_telemetry;
    auto reader = telemetry->f_reader;

    for (auto fid : reader->fieldNames.keys()) {
        QVector<QPointF> *d = reader->fieldData.value(fid);
        if (!d)
            continue;

        /*
            bool is_all_zero = true;
        for (auto const &p : *d) {
            if (p.y() != 0.0) {
                is_all_zero = false;
                break;
            }
        }
        if (is_all_zero)
            continue;
        */

        const QString &s = reader->fieldNames.value(fid);

        MandalaFact *f = qobject_cast<MandalaFact *>(
            Vehicles::instance()->f_replay->f_mandala->findChild(s));

        m_name_data.insert(s, d);
        m_uid_data.insert(f->uid(), d);
    }
}

void TelemetryExtractor::clear()
{
    m_name_data.clear();
    m_uid_data.clear();
}

std::optional<QVector<QPointF> *> TelemetryExtractor::by_id(quint64 uid)
{
    auto it = m_uid_data.find(uid);

    if (it == m_uid_data.end() || it.value() == nullptr)
        return std::nullopt;

    return it.value();
}

std::optional<QVector<QPointF> *> TelemetryExtractor::by_name(QString name)
{
    auto it = m_name_data.find(name);

    if (it == m_name_data.end() || it.value() == nullptr)
        return std::nullopt;

    return it.value();
}
