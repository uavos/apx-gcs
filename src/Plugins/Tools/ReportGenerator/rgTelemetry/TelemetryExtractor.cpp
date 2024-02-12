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
    clear_synced_data();

    std::for_each(m_reader->fieldNames.constKeyValueBegin(),
                  m_reader->fieldNames.constKeyValueEnd(),
                  [this](auto it) {
                      auto fid = it.first;
                      const QString &s = it.second;

                      if (auto *d = m_reader->fieldData.value(fid)) {
                          MandalaFact *f = qobject_cast<MandalaFact *>(
                              Vehicles::instance()->f_replay->f_mandala->findChild(s));

                          std::get<0>(data).insert(f->uid(), d);
                          std::get<1>(data).insert(s, d);
                      }
                  });
}

void TelemetryExtractor::clear_synced_data()
{
    std::get<0>(data).clear();
    std::get<1>(data).clear();
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
