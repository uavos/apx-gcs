#include "TelemetryResolver.h"
#include "TelemetryExtractor.h"

TelemetryResolver::TelemetryResolver(TelemetryExtractor &extractor)
    : m_extractor{extractor}
{
    set_calc_function("MAX_ALTITUDE", [&]() -> QString {
        auto altitude_opt = m_extractor.by_id(mandala::est::nav::pos::altitude::uid);

        if (!altitude_opt.has_value())
            return "__NOVALUE";

        auto &&altitude = altitude_opt.value();

        auto max_altitude
            = std::max_element(altitude->begin(), altitude->end(), [](auto lhs, auto rhs) {
                  return lhs.y() < rhs.y();
              })->y();

        return QString::number(max_altitude);
    });
}

std::optional<QString> TelemetryResolver::get_value(QString field_name)
{
    auto it = m_data.find(field_name);

    if (it == m_data.end() || it.value() == nullptr)
        return std::nullopt;

    return it.value()();
}

void TelemetryResolver::set_calc_function(QString field_name, std::function<QString()> callback)
{
    auto it = m_data.find(field_name);

    if (it == m_data.end() || it.value() == nullptr) {
        m_data.insert(it, field_name, callback);
        return;
    }

    qWarning() << "TelemetryResolver: Collision detected, rewriting current";
    m_data.insert(it, field_name, callback);
}

void TelemetryResolver::clear()
{
    m_data.clear();
}
