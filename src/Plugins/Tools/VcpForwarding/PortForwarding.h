#pragma once

#include <App/AppGcs.h>
#include <Vehicles/Vehicle.h>

class PortForwarding : public Fact
{
    Q_OBJECT

public:
    explicit PortForwarding(QObject *parent,
                            const QString &name,
                            const QString &title,
                            const QString &descr,
                            const QString &icon);

    Fact *f_enabled;
    Fact *f_vcpid;

protected:
    PData *_pdata{};

    virtual bool forwardToPHY(QByteArray data) = 0;

private slots:
    void onCurrentVehicleChanged();
    void onPdataSerialData(quint8 portID, QByteArray data);
    void updateStatus();
};
