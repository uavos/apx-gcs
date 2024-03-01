#include "PortForwarding.h"

PortForwarding::PortForwarding(QObject *parent,
                               const QString &name,
                               const QString &title,
                               const QString &descr,
                               const QString &icon)
    : Fact(parent, name, title, descr, Group, icon)
{
    f_enabled
        = new Fact(this, "enable", tr("Enable"), tr("Enable data forwarding"), Fact::Bool, "link");

    f_vcpid = new Fact(this,
                       "vcpid",
                       tr("VCP"),
                       tr("Virtual Port Number (ID)"),
                       Fact::Int | Fact::PersistentValue,
                       "numeric");
    f_vcpid->setMin(0);
    f_vcpid->setMax(255);

    // allow reception of data from any UDP port
    connect(Vehicles::instance(),
            &Vehicles::currentChanged,
            this,
            &PortForwarding::onCurrentVehicleChanged);
    onCurrentVehicleChanged();

    connect(this, &Fact::activeChanged, this, &PortForwarding::updateStatus);
}

void PortForwarding::onCurrentVehicleChanged()
{
    if (_pdata) {
        disconnect(_pdata, &PData::serialData, this, &PortForwarding::onPdataSerialData);
    }
    auto protocol = Vehicles::instance()->current()->protocol();
    if (protocol)
        _pdata = protocol->data();
    if (_pdata) {
        connect(_pdata, &PData::serialData, this, &PortForwarding::onPdataSerialData);
    }
}

void PortForwarding::onPdataSerialData(quint8 portID, QByteArray data)
{
    if (!f_enabled->value().toBool())
        return;

    if (data.isEmpty())
        return;

    if (portID != f_vcpid->value().toInt())
        return;

    if (forwardToPHY(data)) {
        setActive(true);
        return;
    }

    apxMsgW() << "VcpForwarding: can't write data to PHY";
    setActive(false);
}

void PortForwarding::updateStatus()
{
    if (active()) {
        setText(QString("VCP#%1").arg(f_vcpid->value().toInt()));
    } else {
        if (!f_enabled->value().toBool()) {
            setText("");
        } else {
            setText("Error");
        }
    }
}
