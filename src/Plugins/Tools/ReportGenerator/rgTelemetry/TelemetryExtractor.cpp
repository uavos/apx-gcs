#include "TelemetryExtractor.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

TelemetryExtractor::TelemetryExtractor()
{
    auto telemetry = Vehicles::instance()->f_replay->f_telemetry;
    m_reader = telemetry->f_reader;

    connect(m_reader,
            &TelemetryReader::dataAvailable,
            this,
            &TelemetryExtractor::telemetry_data_changed);
}

void TelemetryExtractor::sync()
{
    clear();

    for (auto fid : m_reader->fieldNames.keys()) {
        QVector<QPointF> *d = m_reader->fieldData.value(fid);
        if (!d)
            continue;

        const QString &s = m_reader->fieldNames.value(fid);

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

TelemetryExtractor &TelemetryExtractor::instance()
{
    static TelemetryExtractor inst;
    return inst;
}

void TelemetryExtractor::telemetry_data_changed()
{
    sync();
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
