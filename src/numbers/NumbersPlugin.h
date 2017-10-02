#ifndef NUMBERSPLUGIN_H
#define NUMBERSPLUGIN_H
#include "plugin_interface.h"
class QmlView;
class QQmlComponent;
class QQuickItem;
//=============================================================================
class NumbersPlugin : public PluginInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.uavos.gcs.PluginInterface/1.0")
  Q_INTERFACES(PluginInterface)
public:
  void init(void);
private:
  QString srcBase;
  QmlView *view;
  QQuickItem *item;
  QStringList getConfig();
  QString content;
private slots:
  void make();
  void configure();
  void saveAs();
  void open();
};
//=============================================================================
#endif // NUMBERSPLUGIN_H
