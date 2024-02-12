#pragma once

#include <Mandala/Mandala.h>
#include <Telemetry/TelemetryReader.h>
#include <optional>
#include <QtCore>

/**
 * @brief Used for convenient access to telemetry data
 * 
 */
class TelemetryExtractor : public QObject
{
    Q_OBJECT

public:
    using name_t = QString;
    using uid_t = quint64;

    static TelemetryExtractor &instance();

    template<typename Key>
    std::optional<QVector<QPointF> *> by(const Key &key);

private slots:
    void telemetry_data_changed();

private:
    TelemetryExtractor();
    void sync();
    void clear_synced_data();

    std::tuple<QMap<quint64, QVector<QPointF> *>, QMap<QString, QVector<QPointF> *>> data;
    TelemetryReader *m_reader;
};

template<typename Key>
inline std::optional<QVector<QPointF> *> TelemetryExtractor::by(const Key &key)
{
    QVector<QPointF> *ret = nullptr;
    if constexpr (std::is_same<Key, uid_t>::value) {
        ret = std::get<0>(data).value(key, nullptr);
    } else if constexpr (std::is_same<Key, name_t>::value) {
        ret = std::get<1>(data).value(key, nullptr);
    }

    if (ret == nullptr)
        return std::nullopt;

    return ret;
}
