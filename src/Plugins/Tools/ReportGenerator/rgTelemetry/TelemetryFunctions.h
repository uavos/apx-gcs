#pragma once

#include "rgTelemetry/TelemetryAccessor.h"
#include "rgUtils/CachedFunction.h"
#include <optional>
#include <QtCore>

namespace ReportGenerator {

#define TelFunction(name, body) \
    TelemetryFunc name = TelemetryFunc(this, #name, [this]() -> QVariant body)

class TelemetryFunctions;

/**
 * @brief Cached function class, using name of function for reflection
 * 
 */
struct TelemetryFunc : public ParamlessCachedFunction<QVariant>
{
    TelemetryFunc(TelemetryFunctions *parent, QString func_name, std::function<QVariant()> func);
};

/**
 * @brief Used to store functions for telemetry resolver
 * 
 */
class TelemetryFunctions : public QObject, public Singleton<TelemetryFunctions>
{
    Q_OBJECT

public:
    TelemetryFunctions();

    TelemetryFunctions(const TelemetryFunctions &) = delete;
    TelemetryFunctions &operator=(const TelemetryFunctions &) = default;

    std::optional<QVariant> call(QString name);

    void clearCache();

private slots:
    void clearCacheSlot();

private:
    QMap<QString, TelemetryFunc *> m_registry;

    TelemetryAccessor &m_ext{TelemetryAccessor::getInstance()};

    // Functions for telemetry calculations
public:
    TelFunction(max_altitude, {
        auto altitude_opt = m_ext.get(mandala::est::nav::pos::altitude::uid);

        if (!altitude_opt.has_value())
            return QVariant();

        auto &&altitude = altitude_opt.value();

        auto max_altitude
            = std::max_element(altitude->begin(), altitude->end(), [](auto lhs, auto rhs) {
                  return lhs.y() < rhs.y();
              })->y();

        return max_altitude;
    });

    TelFunction(max_speed, {
        auto speed_opt = m_ext.get(mandala::est::nav::pos::speed::uid);

        if (!speed_opt.has_value())
            return QVariant();

        auto &&speed = speed_opt.value();

        auto max = std::max_element(speed->begin(), speed->end(), [](auto lhs, auto rhs) {
                       return lhs.y() < rhs.y();
                   })->y();

        return max;
    });

    friend class TelemetryFunc;
};

}; // namespace ReportGenerator
