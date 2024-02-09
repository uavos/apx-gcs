#pragma once

#include "rgTelemetry/TelemetryExtractor.h"
#include "rgUtils/CachedFunction.h"
#include <optional>
#include <QtCore>

#define TelFunction(name, body) \
    TelemetryFunc name = TelemetryFunc(this, #name, [this]() -> QVariant body)

class TelemetryFuncRegistry;

/**
 * @brief Cached function class, using name of function for reflection
 * 
 */
struct TelemetryFunc : public CachedFunction<QVariant>
{
    TelemetryFunc(TelemetryFuncRegistry *parent, QString func_name, std::function<QVariant()> func);
};

/**
 * @brief Used to store functions for telemetry resolver
 * 
 */
class TelemetryFuncRegistry : public QObject
{
    Q_OBJECT

public:
    static TelemetryFuncRegistry &instance();

    std::optional<QVariant> call_by_name(QString name);

    void clear_cache();

private slots:
    void telemetry_data_changed();

private:
    TelemetryFuncRegistry();

    QMap<QString, TelemetryFunc *> m_registry;

    TelemetryExtractor &m_ext{TelemetryExtractor::instance()};

    // Functions for telemetry calculations
public:
    TelFunction(max_altitude, {
        auto altitude_opt = m_ext.by_id(mandala::est::nav::pos::altitude::uid);

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
        auto speed_opt = m_ext.by_id(mandala::est::nav::pos::speed::uid);

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
