#pragma once

#include <Mandala/Mandala.h>
#include <optional>
#include <rgUtils/Singleton.h>
#include <QtCore>

class TelemetryReader;

namespace RG {

/**
 * @brief Used for convenient access to telemetry data
 * 
 */
class TelemetryAccessor : public QObject, public Singleton<TelemetryAccessor>
{
    Q_OBJECT

public:
    TelemetryAccessor();

    std::optional<QVector<QPointF> *> get(uid_t uid);

private slots:
    void updateTelemetryDataSlot();

private:
    void sync();
    void clearSyncedData();

    QMap<quint64, QVector<QPointF> *> m_synced_data;
    TelemetryReader *m_reader;
};

}; // namespace RG
