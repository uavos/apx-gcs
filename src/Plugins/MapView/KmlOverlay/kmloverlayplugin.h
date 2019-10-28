#ifndef KMLOVERLAYPLUGIN_H
#define KMLOVERLAYPLUGIN_H

#include "kmloverlay.h"
#include <App/PluginInterface.h>
#include <QtCore>

class KmlOverlayPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.uavos.gcs.PluginInterface/1.0")
    Q_INTERFACES(PluginInterface)
public:
    int flags() override { return Feature | Map; }
    QObject *createControl() override { return new KmlOverlay(); }
    QStringList depends() override { return QStringList() << "MissionPlanner"; }
};

#endif //KMLOVERLAYPLUGIN_H
