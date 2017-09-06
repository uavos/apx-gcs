#ifndef VideocvPLUGIN_H
#define VideocvPLUGIN_H
#include "plugin_interface.h"
//=============================================================================
class VideocvPlugin : public PluginInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.uavos.gcu.PluginInterface/1.0")
  Q_INTERFACES(PluginInterface)
public:
  void init(void);
};
//=============================================================================
#endif // VideocvPLUGIN_H
