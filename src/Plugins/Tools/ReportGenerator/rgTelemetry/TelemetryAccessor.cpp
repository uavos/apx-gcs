#include "TelemetryAccessor.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

namespace ReportGenerator {

TelemetryAccessor::TelemetryAccessor()
{
    auto telemetry = Vehicles::instance()->f_replay->f_telemetry;
    m_reader = telemetry->f_reader;

    connect(m_reader,
            &TelemetryReader::dataAvailable,
            this,
            &TelemetryAccessor::updateTelemetryDataSlot);
}

std::optional<QVector<QPointF> *> TelemetryAccessor::get(uid_t uid)
{
    auto it = m_synced_data.find(uid);

    if (it == m_synced_data.end() || it.value() == nullptr)
        return std::nullopt;

    return it.value();
}

void TelemetryAccessor::sync()
{
    std::for_each(m_reader->fieldNames.constKeyValueBegin(),
                  m_reader->fieldNames.constKeyValueEnd(),
                  [this](auto it) {
                      auto fid = it.first;
                      const QString &s = it.second;

                      if (auto *d = m_reader->fieldData.value(fid)) {
                          MandalaFact *f = qobject_cast<MandalaFact *>(
                              Vehicles::instance()->f_replay->f_mandala->findChild(s));

                          m_synced_data.insert(f->uid(), d);
                      }
                  });
}

void TelemetryAccessor::clearSyncedData()
{
    m_synced_data.clear();
}

void TelemetryAccessor::updateTelemetryDataSlot()
{
    clearSyncedData();
    sync();
}

}; // namespace ReportGenerator
