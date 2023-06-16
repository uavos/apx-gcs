#pragma once

#include <App/PluginInterface.h>
#include <QtCore>
#include "PortForwarding.h"

class PortForwardingPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.PluginInterface/1.0")
    Q_INTERFACES(PluginInterface)
public:
    int flags() override { return Feature | Tool; }
    QObject *createControl() override { return new PortForwarding(); }
};
