#include "VcpForwarding.h"

#include "SerialForwarding.h"
#include "UdpForwarding.h"

VcpForwarding::VcpForwarding(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("VCP forwarding"),
           tr("Forward system PHY to APX virtual port"),
           Group,
           "swap-vertical")
{
    new SerialForwarding(this);
    new UdpForwarding(this);
}
