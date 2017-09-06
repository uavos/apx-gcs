#include "Config.h"
#include "QMandala.h"
#include <QtCore>
#include <QComboBox>
#include <QSerialPortInfo>
//=============================================================================
Config::Config(QWidget *parent) :
    QDialog(parent)
{
  setupUi(this);
  buttonBox->addButton(tr("Edit.."),QDialogButtonBox::NoRole);
  cbMasterHost->addItems(QSettings().value("opts_masterHost").toStringList());
  load();
}
//=============================================================================
void Config::on_buttonBox_clicked(QAbstractButton *button)
{
  switch(buttonBox->standardButton(button)){
  case QDialogButtonBox::RestoreDefaults: defaults();load();break;
  case QDialogButtonBox::Ok: save();accept();break;
  case QDialogButtonBox::Cancel: reject();break;
  case QDialogButtonBox::NoButton:
    QProcess::startDetached("kate",QStringList()<<(QSettings().fileName()));
    reject();
    break;
  default: ;
  }
}
//=============================================================================
void Config::load()
{
  QSettings st;
  ePort1->setEditText(st.value("serial1").toString());
  ePort2->clear();
  ePort1->clear();
  ePort1->addItem("auto");
  ePort1->addItem("disabled");
#ifdef Q_OS_MAC
#else
  ePort1->addItem("/dev/ttyUSB0");
  ePort1->addItem("/dev/ttyUSB1");
  ePort1->addItem("ttyACM0");
  ePort1->addItem("rfcomm0");
#endif
  foreach(QSerialPortInfo spi,QSerialPortInfo::availablePorts()){
    ePort1->addItem(spi.portName());
  }
  for(int i=0;i<ePort1->count();i++)
    ePort2->addItem(ePort1->itemText(i));
  ePort2->setEditText(st.value("serial2").toString());
  eBaud1->setEditText(st.value("serial1_baudrate").toString());
  eBaud2->setEditText(st.value("serial2_baudrate").toString());
  eAllowExt->setChecked(st.value("extctrEnabled").toBool());
  eServerName->setText(st.value("serverName").toString());
  eServerPass->setText(st.value("serverPass").toString());
  cbMasterHost->setEditText(st.value("masterHost").toString());
  eMasterHostPass->setText(st.value("masterHostPass").toString());
  eProxy->setText(st.value("proxy").toString());
  eOpenGL->setChecked(st.value("opengl").toBool());
  eMulti->setChecked(st.value("multipleInstances").toBool());

  //languages
  QDir langp(QMandala::Global::lang());
  eLang->clear();
  eLang->addItem("default");
  foreach(QFileInfo f,langp.entryInfoList(QStringList()<<"*.qm"))
    eLang->addItem(f.baseName());
  int eLi=eLang->findText(st.value("lang").toString());
  eLang->setCurrentIndex(eLi>=0?eLi:0);

  //speech
  QDir voicep(QMandala::Global::res());
  voicep.cd("sounds");
  voicep.cd("speech");
  eVoice->clear();
  eVoice->addItem("default");
  foreach(QString s,voicep.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
    eVoice->addItem(s);
  int eVi=eVoice->findText(st.value("voice").toString());
  eVoice->setCurrentIndex(eVi>=0?eVi:0);

  //ip address completers
}
//=============================================================================
void Config::defaults(bool force)
{
  QSettings st;
  if(force) st.clear();
  if(force||(!st.contains("serial1"))) st.setValue("serial1","auto");
  if(force||(!st.contains("serial2"))) st.setValue("serial2","disabled");
  if(force||(!st.contains("serial1_baudrate"))) st.setValue("serial1_baudrate",460800);
  if(force||(!st.contains("serial2_baudrate"))) st.setValue("serial2_baudrate",460800);
  if(force||(!st.contains("masterHost"))) st.setValue("masterHost","");
  if(force||(!st.contains("masterHostPass"))) st.setValue("masterHostPass","");
  if(force||(!st.contains("extctrEnabled"))) st.setValue("extctrEnabled",true);
  if(force||(!st.contains("serverPass"))) st.setValue("serverPass","");
  if(force||(!st.contains("opengl"))) st.setValue("opengl",false);
  if(force||(!st.contains("sounds"))) st.setValue("sounds",true);
  if(force) st.remove("windowsState");
}
//=============================================================================
void Config::save()
{
  QSettings st;
  st.setValue("serial1",ePort1->currentText());
  st.setValue("serial2",ePort2->currentText());
  st.setValue("serial1_baudrate",eBaud1->currentText());
  st.setValue("serial2_baudrate",eBaud2->currentText());
  st.setValue("masterHost",cbMasterHost->currentText());
  st.setValue("masterHostPass",eMasterHostPass->text());
  st.setValue("extctrEnabled",eAllowExt->isChecked());
  st.setValue("serverName",eServerName->text());
  st.setValue("serverPass",eServerPass->text());
  st.setValue("proxy",eProxy->text());
  st.setValue("opengl",eOpenGL->isChecked());
  st.setValue("multipleInstances",eMulti->isChecked());
  st.setValue("lang",eLang->currentText());
  st.setValue("voice",eVoice->currentText());

  //ip address completers
  QStringList sc(st.value("opts_masterHost").toStringList());
  sc.prepend(cbMasterHost->currentText());
  sc.removeAll("");
  sc.removeDuplicates();
  while(sc.size()>3)sc.removeLast();
  st.setValue("opts_masterHost",sc);

}
//=============================================================================
//=============================================================================
void Config::on_eLang_currentIndexChanged(const QString &text)
{
  eLang->setToolTip("");
  if(text.contains("by"))
    eLang->setToolTip("Mihas Varantsou (meequz@gmail.com)");
}
//=============================================================================

