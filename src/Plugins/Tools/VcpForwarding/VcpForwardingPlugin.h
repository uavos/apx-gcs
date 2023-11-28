#pragma once

#include "VcpForwarding.h"
#include <App/PluginInterface.h>
#include <QtCore>

class VcpForwardingPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.PluginInterface/1.0")
    Q_INTERFACES(PluginInterface)
public:
    int flags() override { return Feature | Tool; }
    QObject *createControl() override { return new VcpForwarding(); }
};
