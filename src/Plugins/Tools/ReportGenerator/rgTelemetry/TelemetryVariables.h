#pragma once

#include "rgCore/CachedVariableFact.h"
#include "rgTelemetry/TelemetryAccessor.h"
#include "rgUtils/CachedFunction.h"
#include <optional>
#include <QtCore>

namespace RG {

/**
 * @brief Used to store variables for telemetry resolver
 * 
 */
class TelemetryVariables : public QObject, public Singleton<TelemetryVariables>
{
    Q_OBJECT

public:
    TelemetryVariables();

    std::optional<FunctionResult> call(QString name);

    void clearCache();

private slots:
    void clearCacheSlot();

private:
    QHash<QString, CachedVariableFact *> m_registry = {{"max_altitude", &max_altitude},
                                                       {"max_speed", &max_speed}};

    TelemetryAccessor &m_telemetry{TelemetryAccessor::getInstance()};

public:
    CachedVariableFact max_altitude{[this]() -> FunctionResult {
        auto altitude_opt = m_telemetry.get(mandala::est::nav::pos::altitude::uid);

        if (!altitude_opt.has_value())
            return FunctionResult(ReturnStatus::CantAccessTelemetryParameter);

        auto &&altitude = altitude_opt.value();

        auto max_altitude
            = std::max_element(altitude->begin(), altitude->end(), [](auto lhs, auto rhs) {
                  return lhs.y() < rhs.y();
              })->y();

        return FunctionResult(max_altitude);
    }};

    CachedVariableFact max_speed{[this]() -> FunctionResult {
        auto speed_opt = m_telemetry.get(mandala::est::nav::pos::speed::uid);

        if (!speed_opt.has_value())
            return FunctionResult(ReturnStatus::CantAccessTelemetryParameter);

        auto &&speed = speed_opt.value();

        auto max = std::max_element(speed->begin(), speed->end(), [](auto lhs, auto rhs) {
                       return lhs.y() < rhs.y();
                   })->y();

        return FunctionResult(max);
    }};
};

}; // namespace RG
